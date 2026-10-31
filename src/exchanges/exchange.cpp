#include "exchange.h"
#include "binance.h"
#include "coinbase.h"
#include "kraken.h"
#include "poloniex.h"
#include "version.h"

#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

static constexpr int kRequestTimeoutMs = 15 * 1000;

Exchange::Exchange(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_network(network)
{
}

QList<QPair<QString, QString>> Exchange::available()
{
    return {
        {"binance_com", "Binance Global"},
        {"binance_us", "Binance US"},
        {"coinbase_com", "Coinbase"},
        {"kraken_com", "Kraken"},
        {"poloniex_com", "Poloniex"},
    };
}

bool Exchange::is_available(const QString &id)
{
    for (const auto &source : available()) {
        if (source.first == id) return true;
    }
    return false;
}

QString Exchange::display_name(const QString &id)
{
    for (const auto &source : available()) {
        if (source.first == id) return source.second;
    }
    return id;
}

Exchange *Exchange::create(const QString &id, QNetworkAccessManager *network, QObject *parent)
{
    if (id == "binance_us") return new BinanceExchange("api.binance.us", network, parent);
    if (id == "coinbase_com") return new CoinbaseExchange(network, parent);
    if (id == "kraken_com") return new KrakenExchange(network, parent);
    if (id == "poloniex_com") return new PoloniexExchange(network, parent);

    return new BinanceExchange("api.binance.com", network, parent);
}

void Exchange::fetch(const QStringList &pairs)
{
    if (m_busy) return;

    if (pairs.isEmpty()) {
        emit finished({});
        return;
    }

    m_busy = true;
    do_fetch(pairs);
}

void Exchange::finish(const TickerMap &tickers)
{
    m_busy = false;
    emit finished(tickers);
}

void Exchange::get(const QUrl &url, Callback callback)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, SOFT_NAME + " " + SOFT_VERSION);
    request.setTransferTimeout(kRequestTimeoutMs);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

    // "this" as context : the callback is dropped if the exchange is destroyed meanwhile
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        Response response;
        response.http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        response.body = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            qInfo() << "Network request error:" << reply->url().toString() << ":" << reply->errorString() << response.body.left(300);
        }

        callback(response);
    });
}

void Exchange::get_each(const QStringList &pairs, std::function<QUrl(const QString &pair)> url_for, Parser parse)
{
    struct State {
        int pending = 0;
        TickerMap tickers;
    };
    auto state = std::make_shared<State>();
    state->pending = pairs.size();

    for (const QString &pair : pairs) {
        get(url_for(pair), [this, state, pair, parse](const Response &response) {
            state->tickers.insert(pair, parse(pair, response));
            if (--state->pending == 0) finish(state->tickers);
        });
    }
}

double Exchange::change_percent(double open, double last)
{
    return open > 0 ? (last - open) / open * 100 : 0;
}

bool Exchange::split_pair(const QString &pair, QString *base, QString *quote)
{
    const QStringList parts = pair.split('-');
    if (parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty()) return false;

    if (base) *base = parts[0];
    if (quote) *quote = parts[1];
    return true;
}

double Exchange::number(const QJsonValue &value)
{
    return value.isString() ? value.toString().toDouble() : value.toDouble();
}

TickerMap Exchange::all(const QStringList &pairs, const Ticker &ticker)
{
    TickerMap tickers;
    for (const QString &pair : pairs) {
        tickers.insert(pair, ticker);
    }
    return tickers;
}
