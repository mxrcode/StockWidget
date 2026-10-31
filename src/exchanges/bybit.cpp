#include "bybit.h"

#include <QJsonArray>
#include <QJsonObject>

#include <memory>

static const QUrl kTickersUrl("https://api.bybit.com/v5/market/tickers?category=spot");

QString BybitExchange::symbol_of(const QString &pair)
{
    QString symbol = pair;
    return symbol.remove('-');
}

Ticker BybitExchange::parse_ticker(const QJsonObject &obj)
{
    // "price24hPcnt" is rounded to 0.01%, calculate the change from the price 24 hours ago instead
    const double last = number(obj["lastPrice"]);
    return Ticker::make(last, number(obj["highPrice24h"]), number(obj["lowPrice24h"]), change_percent(number(obj["prevPrice24h"]), last));
}

QJsonArray BybitExchange::result_list(const Response &response, bool *ok)
{
    const QJsonObject json = response.json().object();
    *ok = response.ok() && json["retCode"].toInt(-1) == 0;
    if (!*ok) {
        qInfo() << "Bybit :" << json["retMsg"].toString();
        return {};
    }
    return json["result"].toObject()["list"].toArray();
}

void BybitExchange::do_fetch(const QStringList &pairs)
{
    get(kTickersUrl, [this, pairs](const Response &response) {
        bool ok = false;
        const QJsonArray list = result_list(response, &ok);
        if (!ok) {
            finish(all(pairs, Ticker::network_error()));
            return;
        }

        QHash<QString, QJsonObject> by_symbol;
        for (const QJsonValue &value : list) {
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

void BybitExchange::load_markets(MarketsCallback callback)
{
    // Symbols are written without a separator ("BTCUSDT"), so the base and quote
    // currencies come from instruments-info, prices from the tickers
    struct State {
        int pending = 2;
        bool ok = true;
        QJsonArray instruments;
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
        for (const QJsonValue &value : state->instruments) {
            const QJsonObject obj = value.toObject();
            if (obj["status"].toString() != "Trading") continue;

            const QJsonObject data = tickers.value(obj["symbol"].toString());
            const Ticker ticker = parse_ticker(data);

            Market market;
            market.pair = obj["baseCoin"].toString() + "-" + obj["quoteCoin"].toString();
            market.last = ticker.last;
            market.change_percent = ticker.change_percent;
            market.quote_volume = number(data["turnover24h"]); // "turnover24h" is in the quote currency
            markets.append(market);
        }
        callback(markets, true);
    };

    // Spot instruments are returned in one page
    get(QUrl("https://api.bybit.com/v5/market/instruments-info?category=spot"), [state, done](const Response &response) {
        bool ok = false;
        state->instruments = result_list(response, &ok);
        state->ok = state->ok && ok;
        done();
    });
    get(kTickersUrl, [state, done](const Response &response) {
        bool ok = false;
        state->tickers = result_list(response, &ok);
        state->ok = state->ok && ok;
        done();
    });
}
