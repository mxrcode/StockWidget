#include "okx.h"

#include <QJsonArray>
#include <QJsonObject>

#include <memory>

void OkxExchange::get_tickers(std::function<void(const QHash<QString, QJsonObject> &tickers, bool ok)> done)
{
    get(QUrl("https://www.okx.com/api/v5/market/tickers?instType=SPOT"), [done](const Response &response) {
        // { "code": "0", "data": [ { "instId": "BTC-USDT", ... } ] }, "code" is not "0" on errors
        const QJsonObject json = response.json().object();
        if (!response.ok() || json["code"].toString() != "0") {
            qInfo() << "OKX :" << json["msg"].toString();
            done({}, false);
            return;
        }

        QHash<QString, QJsonObject> tickers;
        for (const QJsonValue &value : json["data"].toArray()) {
            const QJsonObject obj = value.toObject();
            tickers.insert(obj["instId"].toString(), obj);
        }
        done(tickers, true);
    });
}

void OkxExchange::do_fetch(const QStringList &pairs)
{
    get_tickers([this, pairs](const QHash<QString, QJsonObject> &by_pair, bool ok) {
        if (!ok) {
            finish(all(pairs, Ticker::network_error()));
            return;
        }

        // OKX uses the same "BTC-USDT" pair names
        TickerMap tickers;
        for (const QString &pair : pairs) {
            if (!by_pair.contains(pair)) {
                tickers.insert(pair, Ticker::unknown_pair());
                continue;
            }

            // "open24h" is the price 24 hours ago, the change is not provided
            const QJsonObject obj = by_pair[pair];
            const double last = number(obj["last"]);
            tickers.insert(pair, Ticker::make(last, number(obj["high24h"]), number(obj["low24h"]), change_percent(number(obj["open24h"]), last)));
        }
        finish(tickers);
    });
}

void OkxExchange::load_markets(MarketsCallback callback)
{
    // Tradable pairs come from /public/instruments (the tickers also list test
    // and suspended instruments), prices from the tickers
    struct State {
        int pending = 2;
        bool ok = true;
        QJsonArray instruments;
        QHash<QString, QJsonObject> tickers;
    };
    auto state = std::make_shared<State>();

    auto done = [state, callback]() {
        if (--state->pending > 0) return;
        if (!state->ok) {
            callback({}, false);
            return;
        }

        QList<Market> markets;
        for (const QJsonValue &value : state->instruments) {
            const QJsonObject instrument = value.toObject();
            if (instrument["state"].toString() != "live") continue;

            const QString pair = instrument["instId"].toString();
            const QJsonObject obj = state->tickers.value(pair);

            Market market;
            market.pair = pair;
            market.last = number(obj["last"]);
            market.change_percent = change_percent(number(obj["open24h"]), market.last);
            market.quote_volume = number(obj["volCcy24h"]); // for spot pairs "volCcy24h" is in the quote currency
            markets.append(market);
        }
        callback(markets, true);
    };

    get(QUrl("https://www.okx.com/api/v5/public/instruments?instType=SPOT"), [state, done](const Response &response) {
        const QJsonObject json = response.json().object();
        state->ok = state->ok && response.ok() && json["code"].toString() == "0";
        state->instruments = json["data"].toArray();
        done();
    });
    get_tickers([state, done](const QHash<QString, QJsonObject> &tickers, bool ok) {
        state->ok = state->ok && ok;
        state->tickers = tickers;
        done();
    });
}
