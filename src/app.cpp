#include "app.h"
#include "autorun.h"
#include "exchanges/exchange.h"
#include "version.h"

#include <QApplication>
#include <QDebug>

static constexpr int kRetryIntervalMs = 15 * 1000; // After a network error

App::App(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &App::refresh);

    connect(&m_configurator, &Configurator::applied, this, [this](const Config &config) {
        apply_config(config);
        save_config();
    });

    connect(&m_widget, &MainWindow::moved, this, [this](const QString &position) {
        m_config.position = position;
        m_configurator.set_position(position);
        save_config();
    });
}

App::~App()
{
    // Before the widget and the network manager are destroyed : no late replies
    qDeleteAll(m_exchanges);
}

void App::start()
{
    m_widget.setWindowTitle(SOFT_NAME + " " + SOFT_VERSION);

    build_tray_menu();
    apply_config(Config::load());
}

void App::build_tray_menu()
{
    // Remove the empty filed for Icons
    m_tray_menu.setStyleSheet("QMenu::item::icon{width: 0px;} QMenu::item::icon:disabled {width: 0px;} QMenu::item::icon:off {width: 0px;} QMenu::item::icon:on {width: 0px;} QMenu::item::icon:selected {width: 0px;} QMenu::item::icon:checked {width: 0px;} QMenu::indicator { width: 0px;}");

    connect(m_tray_menu.addAction("Edit Config"), &QAction::triggered, this, &App::show_configurator);

    connect(m_tray_menu.addAction("Restart App"), &QAction::triggered, this, [this]() {
        m_restart_requested = true;
        qApp->quit();
    });

    QAction *autorun_action = m_tray_menu.addAction("Autorun");
    autorun_action->setCheckable(true);
    autorun_action->setChecked(Autorun::is_enabled());
    connect(autorun_action, &QAction::toggled, this, &Autorun::set_enabled);

    m_always_on_top_action = m_tray_menu.addAction("Always on top");
    m_always_on_top_action->setCheckable(true);
    connect(m_always_on_top_action, &QAction::triggered, this, [this](bool checked) {
        m_config.always_on_top = checked;
        m_widget.set_always_on_top(checked);
        save_config();
    });

    m_tray_menu.addSeparator();

    connect(m_tray_menu.addAction("About Me"), &QAction::triggered, &m_about_me, &QWidget::show);
    connect(m_tray_menu.addAction("About Qt"), &QAction::triggered, &m_about_qt, &QWidget::show);

    m_tray_menu.addSeparator();

    connect(m_tray_menu.addAction("Close"), &QAction::triggered, qApp, &QApplication::quit);

    // Show Configurator
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason != QSystemTrayIcon::Trigger) return;

        if (m_configurator.isHidden()) {
            show_configurator();
        } else {
            m_configurator.hide();
        }
    });

    m_tray.setContextMenu(&m_tray_menu);
    m_tray.setIcon(QIcon(":/img/icon.svg"));
    m_tray.setToolTip(SOFT_NAME + " " + SOFT_VERSION);
    m_tray.show();
}

void App::show_configurator()
{
    m_configurator.set_config(m_config);
    m_configurator.show();
    m_configurator.activateWindow();
    m_configurator.raise();
}

void App::apply_config(const Config &config)
{
    const bool pairs_changed = !m_config_applied || config.pairs != m_config.pairs || config.text_style != m_config.text_style;
    const bool interval_changed = config.refresh_interval != m_config.refresh_interval;

    m_config = config;
    m_config_applied = true;
    if (!Exchange::is_available(m_config.data_source)) m_config.data_source = "binance_com";

    m_widget.set_color_changes(m_config.color_changes);
    if (pairs_changed) {
        m_widget.set_pairs(m_config.pairs, m_config.text_style);
    }
    m_widget.place(m_config.position);

    m_always_on_top_action->setChecked(m_config.always_on_top);
    m_widget.set_always_on_top(m_config.always_on_top); // Also shows the widget

    m_reported_unknown_pairs.clear();

    if (pairs_changed) {
        // Results of the requests still in flight are for the old pairs
        recreate_exchanges();
        m_timer.stop();
        refresh();
    } else if (interval_changed && m_pending_exchanges == 0) {
        m_timer.start(m_config.refresh_interval * 1000);
    }
}

void App::save_config()
{
    if (m_config.save()) return;

    m_warning.setTextBrowser("Unknown error when trying to write file \"" + Config::file_path() + "\". Make sure that you have permission to write to it.");
    m_warning.show();
}

void App::delete_exchanges()
{
    for (Exchange *exchange : std::as_const(m_exchanges)) {
        exchange->disconnect(this);
        exchange->deleteLater();
    }
    m_exchanges.clear();
    m_pending_exchanges = 0;
}

void App::recreate_exchanges()
{
    delete_exchanges();

    for (const QString &key : std::as_const(m_config.pairs)) {
        const QString source = Config::source_of(key);
        if (m_exchanges.contains(source) || !Exchange::is_available(source)) continue;

        Exchange *exchange = Exchange::create(source, &m_network, this);
        connect(exchange, &Exchange::finished, this, [this, source](const TickerMap &tickers) {
            on_exchange_finished(source, tickers);
        });
        m_exchanges.insert(source, exchange);
    }
}

void App::refresh()
{
    // The timer is restarted when the current round finishes
    if (m_pending_exchanges > 0) return;

    QHash<QString, QStringList> pairs_by_source;
    m_round_tickers.clear();

    for (const QString &key : std::as_const(m_config.pairs)) {
        const QString source = Config::source_of(key);
        if (m_exchanges.contains(source)) {
            pairs_by_source[source].append(Config::pair_of(key));
        } else {
            m_round_tickers.insert(key, Ticker::unknown_pair()); // Misspelled exchange in the config
        }
    }

    if (pairs_by_source.isEmpty()) {
        on_round_finished();
        return;
    }

    m_pending_exchanges = pairs_by_source.size();
    for (auto it = pairs_by_source.constBegin(); it != pairs_by_source.constEnd(); ++it) {
        m_exchanges[it.key()]->fetch(it.value());
    }
}

void App::on_exchange_finished(const QString &source, const TickerMap &tickers)
{
    for (auto it = tickers.constBegin(); it != tickers.constEnd(); ++it) {
        m_round_tickers.insert(Config::pair_key(source, it.key()), it.value());
    }

    if (--m_pending_exchanges == 0) on_round_finished();
}

void App::on_round_finished()
{
    m_widget.show_tickers(m_round_tickers);

    bool network_error = false;
    QStringList new_unknown_pairs;

    for (auto it = m_round_tickers.constBegin(); it != m_round_tickers.constEnd(); ++it) {
        if (it.value().status == Ticker::Status::NetworkError) {
            network_error = true;
        } else if (it.value().status == Ticker::Status::UnknownPair && !m_reported_unknown_pairs.contains(it.key())) {
            m_reported_unknown_pairs.insert(it.key());
            new_unknown_pairs.append(Config::pair_of(it.key()) + " (" + Exchange::display_name(Config::source_of(it.key())) + ")");
        }
    }

    if (!new_unknown_pairs.isEmpty()) {
        qInfo() << "Pairs not found :" << new_unknown_pairs;

        if (m_config.error_notifications) {
            m_tray.showMessage("Error", "Some trading pairs were not found on the exchange: " + new_unknown_pairs.join(", ") + ". Double-check symbols and make sure these pairs exist and are available for trading on your chosen exchange.", QIcon(":/img/icon.svg"), 5000);
        }
    }

    const int interval_ms = m_config.refresh_interval * 1000;
    m_timer.start(network_error ? qMin(kRetryIntervalMs, interval_ms) : interval_ms);
}
