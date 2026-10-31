#include "coinbase.h"

#include <QJsonArray>
#include <QJsonObject>

#include <memory>

static const QUrl kStatsUrl("https://api.exchange.coinbase.com/products/stats");

void CoinbaseExchange::do_fetch(const QStringList &pairs)
{
    get(kStatsUrl, [this, pairs](const Response &response) {
        if (!response.ok()) {
            finish(all(pairs, Ticker::network_error()));
            return;
        }

        // { "BTC-USD": { "stats_24hour": { "open", "high", "low", "last", ... } } }
        const QJsonObject products = response.json().object();

        TickerMap tickers;
        for (const QString &pair : pairs) {
            const QJsonObject stats = products[pair].toObject()["stats_24hour"].toObject();
            if (stats.isEmpty()) {
                tickers.insert(pair, Ticker::unknown_pair());
                continue;
            }

            // Coinbase does not provide the change, calculate it from the 24h open price
            const double last = number(stats["last"]);
            tickers.insert(pair, Ticker::make(last, number(stats["high"]), number(stats["low"]), change_percent(number(stats["open"]), last)));
        }
        finish(tickers);
    });
}

void CoinbaseExchange::load_markets(MarketsCallback callback)
{
    // Tradable pairs come from /products, prices from /products/stats
    struct State {
        int pending = 2;
        bool ok = true;
        QJsonArray products;
        QJsonObject stats;
    };
    auto state = std::make_shared<State>();

    auto done = [state, callback]() {
        if (--state->pending > 0) return;
        if (!state->ok) {
            callback({}, false);
            return;
        }

        QList<Market> markets;
        for (const QJsonValue &value : state->products) {
            const QJsonObject product = value.toObject();
            if (product["status"].toString() != "online" || product["trading_disabled"].toBool()) continue;

            const QString pair = product["id"].toString();
            const QJsonObject stats = state->stats[pair].toObject()["stats_24hour"].toObject();

            Market market;
            market.pair = pair;
            market.last = number(stats["last"]);
            market.change_percent = change_percent(number(stats["open"]), market.last);
            market.quote_volume = number(stats["volume"]) * market.last;
            markets.append(market);
        }
        callback(markets, true);
    };

    get(QUrl("https://api.exchange.coinbase.com/products"), [state, done](const Response &response) {
        state->ok = state->ok && response.ok();
        state->products = response.json().array();
        done();
    });
    get(kStatsUrl, [state, done](const Response &response) {
        state->ok = state->ok && response.ok();
        state->stats = response.json().object();
        done();
    });
}
