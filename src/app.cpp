#include "app.h"
#include "autorun.h"
#include "exchanges/exchange.h"
#include "version.h"

#include <QApplication>
#include <QDebug>

static constexpr int kRefreshIntervalMs = 60 * 1000;
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
}

App::~App()
{
    // Before the widget and the network manager are destroyed : no late replies
    delete m_exchange;
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
    const bool source_changed = !m_exchange || config.data_source != m_config.data_source;
    const bool pairs_changed = config.pairs != m_config.pairs || config.text_style != m_config.text_style;

    m_config = config;
    if (!Exchange::is_available(m_config.data_source)) m_config.data_source = "binance_com";

    if (source_changed) {
        if (m_exchange) {
            // Results of requests still in flight are no longer needed
            m_exchange->disconnect(this);
            m_exchange->deleteLater();
        }
        m_exchange = Exchange::create(m_config.data_source, &m_network, this);
        connect(m_exchange, &Exchange::finished, this, &App::on_tickers);
    }

    if (source_changed || pairs_changed || m_widget.isHidden()) {
        m_widget.set_pairs(m_config.pairs, m_config.text_style);
    }
    m_widget.place(m_config.position);

    m_always_on_top_action->setChecked(m_config.always_on_top);
    m_widget.set_always_on_top(m_config.always_on_top); // Also shows the widget

    m_reported_unknown_pairs.clear();

    if (source_changed || pairs_changed) {
        m_timer.stop();
        refresh();
    }
}

void App::save_config()
{
    if (m_config.save()) return;

    m_warning.setTextBrowser("Unknown error when trying to write file \"" + Config::file_path() + "\". Make sure that you have permission to write to it.");
    m_warning.show();
}

void App::refresh()
{
    // A refresh is already running, the timer is restarted when it finishes
    if (m_exchange->is_busy()) return;

    m_exchange->fetch(m_config.pairs);
}

void App::on_tickers(const TickerMap &tickers)
{
    m_widget.show_tickers(tickers);

    bool network_error = false;
    QStringList new_unknown_pairs;

    for (auto it = tickers.constBegin(); it != tickers.constEnd(); ++it) {
        if (it.value().status == Ticker::Status::NetworkError) {
            network_error = true;
        } else if (it.value().status == Ticker::Status::UnknownPair && !m_reported_unknown_pairs.contains(it.key())) {
            new_unknown_pairs.append(it.key());
            m_reported_unknown_pairs.insert(it.key());
        }
    }

    if (!new_unknown_pairs.isEmpty()) {
        qInfo() << "Pairs not found on" << m_config.data_source << ":" << new_unknown_pairs;

        if (m_config.error_notifications) {
            m_tray.showMessage("Error", "Some trading pairs were not found on the exchange (" + new_unknown_pairs.join(", ") + "). Double-check symbols and make sure these pairs exist and are available for trading on your chosen exchange.", QIcon(":/img/icon.svg"), 5000);
        }
    }

    m_timer.start(network_error ? kRetryIntervalMs : kRefreshIntervalMs);
}
