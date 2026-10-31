#include "binance.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

#include <memory>

// Binance answers 400 with this code for the whole request if one of the symbols does not exist
static constexpr int kInvalidSymbolCode = -1121;

BinanceExchange::BinanceExchange(const QString &domain, QNetworkAccessManager *network, QObject *parent)
    : Exchange(network, parent)
    , m_domain(domain)
{
}

QString BinanceExchange::symbol_of(const QString &pair)
{
    QString symbol = pair;
    return symbol.remove('-');
}

Ticker BinanceExchange::parse_ticker(const QJsonObject &obj)
{
    return Ticker::make(number(obj["lastPrice"]), number(obj["highPrice"]), number(obj["lowPrice"]), number(obj["priceChangePercent"]));
}

void BinanceExchange::do_fetch(const QStringList &pairs)
{
    QJsonArray symbols;
    for (const QString &pair : pairs) {
        symbols.append(symbol_of(pair));
    }

    QUrl url("https://" + m_domain + "/api/v3/ticker/24hr");
    QUrlQuery query;
    query.addQueryItem("symbols", QJsonDocument(symbols).toJson(QJsonDocument::Compact));
    url.setQuery(query);

    get(url, [this, pairs](const Response &response) {
        if (response.http_status == 400 && response.json().object()["code"].toInt() == kInvalidSymbolCode) {
            // Find out which pairs are missing
            fetch_one_by_one(pairs);
            return;
        }

        if (!response.ok()) {
            finish(all(pairs, Ticker::network_error()));
            return;
        }

        QHash<QString, QJsonObject> by_symbol;
        for (const QJsonValue &value : response.json().array()) {
            const QJsonObject obj = value.toObject();
            by_symbol.insert(obj["symbol"].toString(), obj);
        }

        TickerMap tickers;
        for (const QString &pair : pairs) {
            const QString symbol = symbol_of(pair);
            tickers.insert(pair, by_symbol.contains(symbol) ? parse_ticker(by_symbol[symbol]) : Ticker::unknown_pair());
        }
        finish(tickers);
    });
}

void BinanceExchange::fetch_one_by_one(const QStringList &pairs)
{
    get_each(pairs, [this](const QString &pair) {
        QUrl url("https://" + m_domain + "/api/v3/ticker/24hr");
        url.setQuery("symbol=" + symbol_of(pair));
        return url;
    }, [](const QString &, const Response &response) {
        if (response.http_status == 400) return Ticker::unknown_pair();
        if (!response.ok()) return Ticker::network_error();
        return parse_ticker(response.json().object());
    });
}

void BinanceExchange::load_markets(MarketsCallback callback)
{
    // Pair names come from exchangeInfo, prices from the "MINI" tickers
    struct State {
        int pending = 2;
        bool ok = true;
        QJsonArray symbols;
        QJsonArray tickers;
    };
    auto state = std::make_shared<State>();

    auto done = [state, callback]() {
        if (--state->pending > 0) return;
        if (!state->ok) {
            callback({}, false);
            return;
        }

        QHash<QString, QJsonObject> tickers;
        for (const QJsonValue &value : state->tickers) {
            const QJsonObject obj = value.toObject();
            tickers.insert(obj["symbol"].toString(), obj);
        }

        QList<Market> markets;
        for (const QJsonValue &value : state->symbols) {
            const QJsonObject obj = value.toObject();
            const QJsonObject ticker = tickers.value(obj["symbol"].toString());

            Market market;
            market.pair = obj["baseAsset"].toString() + "-" + obj["quoteAsset"].toString();
            market.last = number(ticker["lastPrice"]);
            market.change_percent = change_percent(number(ticker["openPrice"]), market.last);
            market.quote_volume = number(ticker["quoteVolume"]);
            markets.append(market);
        }
        callback(markets, true);
    };

    get(QUrl("https://" + m_domain + "/api/v3/exchangeInfo?permissions=SPOT&symbolStatus=TRADING"), [state, done](const Response &response) {
        state->ok = state->ok && response.ok();
        state->symbols = response.json().object()["symbols"].toArray();
        done();
    });
    get(QUrl("https://" + m_domain + "/api/v3/ticker/24hr?type=MINI"), [state, done](const Response &response) {
        state->ok = state->ok && response.ok();
        state->tickers = response.json().array();
        done();
    });
}
