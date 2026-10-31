#ifndef CONFIG_H
#define CONFIG_H

#include <QPoint>
#include <QSet>
#include <QString>
#include <QStringList>

// Widget settings stored in CONFIG_NAME next to the executable.
// File format: "$key:value" lines for options, one trading pair per line.
// A pair line may name its exchange ("kraken_com:ETH-EUR"), otherwise $data_sources is used.
// Pairs hidden from the widget are kept as "#off:" lines ("#off:BTC-USDT"), older versions skip them.
struct Config
{
    static constexpr int kMinRefreshInterval = 10;   // seconds
    static constexpr int kMaxRefreshInterval = 3600;

    QString text_style = "color:rgba(255, 255, 255, 0.65);";
    QString position = "1,0"; // Screen corner "row,column" or "@x,y", see MainWindow::place()
    bool always_on_top = false;
    QString data_source = "binance_com"; // Default exchange
    bool error_notifications = true;
    bool color_changes = true;
    int refresh_interval = 60; // seconds
    QStringList pairs; // Keys "exchange:PAIR", see pair_key()
    QSet<QString> hidden_pairs; // Keys from pairs that are not shown on the widget

    // Pairs shown on the widget, in order
    QStringList shown_pairs() const;

    static QString file_path();

    // Reads the config file. If it does not exist, a default one is created.
    static Config load();
    bool save() const;

    // "btc_usdt" -> "BTC-USDT"
    static QString normalize_pair(QString pair);

    // "kraken_com", "BTC-USD" <-> "kraken_com:BTC-USD"
    static QString pair_key(const QString &source, const QString &pair);
    static QString source_of(const QString &key);
    static QString pair_of(const QString &key);

    // "@120,800" <-> QPoint(120, 800)
    static bool is_free_position(const QString &position);
    static QPoint free_position(const QString &position);
    static QString make_free_position(const QPoint &point);
};

#endif // CONFIG_H
