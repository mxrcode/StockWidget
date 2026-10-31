#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>

// Widget settings stored in CONFIG_NAME next to the executable.
// File format: "$key:value" lines for options, one trading pair per line.
struct Config
{
    QString text_style = "color:rgba(255, 255, 255, 0.65);";
    QString position = "1,0"; // "row,column" of the screen corner, see MainWindow::place()
    bool always_on_top = false;
    QString data_source = "binance_com";
    bool error_notifications = true;
    QStringList pairs;

    static QString file_path();

    // Reads the config file. If it does not exist, a default one is created.
    static Config load();
    bool save() const;

    // "btc_usdt" -> "BTC-USDT"
    static QString normalize_pair(QString pair);
    static QStringList normalize_pairs(const QStringList &pairs);
};

#endif // CONFIG_H
