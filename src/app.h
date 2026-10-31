#ifndef APP_H
#define APP_H

#include "aboutme.h"
#include "aboutqt.h"
#include "config.h"
#include "configurator.h"
#include "mainwindow.h"
#include "ticker.h"
#include "warningui.h"

#include <QHash>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QSystemTrayIcon>
#include <QTimer>

class Exchange;

// Owns the widget, the tray menu and the windows, and refreshes the data periodically
class App : public QObject
{
    Q_OBJECT

public:
    explicit App(QObject *parent = nullptr);
    ~App();

    void start();
    bool restart_requested() const { return m_restart_requested; }

private:
    void build_tray_menu();
    void show_configurator();

    // Applies the settings to the running widget
    void apply_config(const Config &config);
    void save_config();

    // One exchange client per data source used by the pairs
    void recreate_exchanges();
    void delete_exchanges();

    // A refresh round requests every exchange, the widget is updated when all of them answered
    void refresh();
    void on_exchange_finished(const QString &source, const TickerMap &tickers);
    void on_round_finished();

    Config m_config;
    bool m_config_applied = false;

    QNetworkAccessManager m_network;
    QHash<QString, Exchange *> m_exchanges;
    QTimer m_timer;
    int m_pending_exchanges = 0;
    TickerMap m_round_tickers; // Keys "exchange:PAIR"

    MainWindow m_widget;
    Configurator m_configurator;
    AboutMe m_about_me;
    AboutQt m_about_qt;
    WarningUi m_warning;

    QSystemTrayIcon m_tray;
    QMenu m_tray_menu;
    QAction *m_always_on_top_action = nullptr;

    QSet<QString> m_reported_unknown_pairs;
    bool m_restart_requested = false;
};

#endif // APP_H
