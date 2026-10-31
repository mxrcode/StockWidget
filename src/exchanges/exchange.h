#ifndef EXCHANGE_H
#define EXCHANGE_H

#include "ticker.h"

#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;

// Trading pair listed on an exchange
struct Market
{
    QString pair; // "BTC-USDT"
    double last = 0;
    double change_percent = 0;
    double quote_volume = 0; // 24h volume in the quote currency, 0 if unknown
};

// Asynchronous client of an exchange public API.
// fetch() requests 24h tickers and emits finished() exactly once per call.
class Exchange : public QObject
{
    Q_OBJECT

public:
    using MarketsCallback = std::function<void(const QList<Market> &markets, bool ok)>;

    explicit Exchange(QNetworkAccessManager *network, QObject *parent = nullptr);

    void fetch(const QStringList &pairs);
    bool is_busy() const { return m_busy; }

    // All trading pairs of the exchange (for the pair search)
    virtual void load_markets(MarketsCallback callback) = 0;

    // Supported data sources : {id used in the config, display name}
    static QList<QPair<QString, QString>> available();
    static bool is_available(const QString &id);
    static QString display_name(const QString &id);
    static Exchange *create(const QString &id, QNetworkAccessManager *network, QObject *parent = nullptr);

signals:
    void finished(const TickerMap &tickers);

protected:
    struct Response {
        int http_status = 0; // 0 : no HTTP response (connection error, timeout)
        QByteArray body;

        bool ok() const { return http_status == 200; }
        QJsonDocument json() const { return QJsonDocument::fromJson(body); }
    };
    using Callback = std::function<void(const Response &)>;
    using Parser = std::function<Ticker(const QString &pair, const Response &)>;

    // Implementations must call finish() when all their requests are done
    virtual void do_fetch(const QStringList &pairs) = 0;
    void finish(const TickerMap &tickers);

    void get(const QUrl &url, Callback callback);

    // One request per pair, finish() after the last reply
    void get_each(const QStringList &pairs, std::function<QUrl(const QString &pair)> url_for, Parser parse);

    // "BTC-USDT" -> "BTC", "USDT". False if the pair is not in BASE-QUOTE form.
    static bool split_pair(const QString &pair, QString *base, QString *quote);

    // Exchanges return numbers both as JSON strings and as numbers
    static double number(const QJsonValue &value);

    static double change_percent(double open, double last);

    static TickerMap all(const QStringList &pairs, const Ticker &ticker);

private:
    QNetworkAccessManager *m_network;
    bool m_busy = false;
};

#endif // EXCHANGE_H
