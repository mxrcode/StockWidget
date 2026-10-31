#include "kraken.h"

#include <QJsonArray>
#include <QJsonObject>

void KrakenExchange::do_fetch(const QStringList &pairs)
{
    if (!m_kraken_names.isEmpty()) {
        fetch_tickers(pairs);
        return;
    }

    load_asset_pairs([this, pairs](bool ok) {
        if (ok) {
            fetch_tickers(pairs);
        } else {
            finish(all(pairs, Ticker::network_error()));
        }
    });
}

QString KrakenExchange::asset_name(const QString &kraken_asset)
{
    if (kraken_asset == "XBT") return "BTC";
    if (kraken_asset == "XDG") return "DOGE";
    return kraken_asset;
}

Ticker KrakenExchange::parse_ticker(const QJsonObject &data)
{
    // Arrays are [today, last 24 hours]; "o" is the opening price of the current day (UTC)
    const double last = number(data["c"].toArray().at(0));
    const double high = number(data["h"].toArray().at(1));
    const double low = number(data["l"].toArray().at(1));

    return Ticker::make(last, high, low, change_percent(number(data["o"]), last));
}

void KrakenExchange::load_asset_pairs(std::function<void(bool ok)> done)
{
    get(QUrl("https://api.kraken.com/0/public/AssetPairs"), [this, done](const Response &response) {
        const QJsonObject result = response.json().object()["result"].toObject();

        if (!response.ok() || result.isEmpty()) {
            done(false);
            return;
        }

        m_kraken_names.clear();
        for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
            // "wsname" : "XBT/USD"
            const QStringList assets = it.value().toObject()["wsname"].toString().split('/');
            if (assets.size() != 2) continue;

            m_kraken_names.insert(asset_name(assets[0]) + "-" + asset_name(assets[1]), it.key());
        }

        done(true);
    });
}

void KrakenExchange::fetch_tickers(const QStringList &pairs)
{
    TickerMap tickers;
    QStringList known_pairs;
    QStringList kraken_names;

    for (const QString &pair : pairs) {
        if (m_kraken_names.contains(pair)) {
            known_pairs.append(pair);
            kraken_names.append(m_kraken_names[pair]);
        } else {
            tickers.insert(pair, Ticker::unknown_pair());
        }
    }

    if (known_pairs.isEmpty()) {
        finish(tickers);
        return;
    }

    get(QUrl("https://api.kraken.com/0/public/Ticker?pair=" + kraken_names.join(',')), [this, tickers, known_pairs](const Response &response) mutable {
        const QJsonObject json = response.json().object();
        const QJsonObject result = json["result"].toObject();

        if (!response.ok() || !json["error"].toArray().isEmpty()) {
            qInfo() << "Kraken :" << json["error"].toArray();

            // A pair may have been delisted, reload the list next time
            m_kraken_names.clear();

            for (const QString &pair : known_pairs) {
                tickers.insert(pair, Ticker::network_error());
            }
            finish(tickers);
            return;
        }

        for (const QString &pair : known_pairs) {
            const QJsonObject data = result[m_kraken_names[pair]].toObject();
            tickers.insert(pair, data.isEmpty() ? Ticker::unknown_pair() : parse_ticker(data));
        }
        finish(tickers);
    });
}

void KrakenExchange::load_markets(MarketsCallback callback)
{
    load_asset_pairs([this, callback](bool ok) {
        if (!ok) {
            callback({}, false);
            return;
        }

        // Without "pair" Kraken returns the tickers of all pairs
        get(QUrl("https://api.kraken.com/0/public/Ticker"), [this, callback](const Response &response) {
            const QJsonObject result = response.json().object()["result"].toObject();
            if (!response.ok() || result.isEmpty()) {
                callback({}, false);
                return;
            }

            QList<Market> markets;
            for (auto it = m_kraken_names.constBegin(); it != m_kraken_names.constEnd(); ++it) {
                const QJsonObject data = result[it.value()].toObject();
                const Ticker ticker = parse_ticker(data);

                Market market;
                market.pair = it.key();
                market.last = ticker.last;
                market.change_percent = ticker.change_percent;
                market.quote_volume = number(data["v"].toArray().at(1)) * ticker.last; // "v" is in the base currency
                markets.append(market);
            }
            callback(markets, true);
        });
    });
}
