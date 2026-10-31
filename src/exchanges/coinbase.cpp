#include "coinbase.h"

#include <QJsonObject>

void CoinbaseExchange::do_fetch(const QStringList &pairs)
{
    get_each(pairs, [](const QString &pair) {
        return QUrl("https://api.exchange.coinbase.com/products/" + pair + "/stats");
    }, [](const QString &, const Response &response) {
        if (response.http_status == 400 || response.http_status == 404) return Ticker::unknown_pair();
        if (!response.ok()) return Ticker::network_error();

        const QJsonObject obj = response.json().object();
        const double open = number(obj["open"]);
        const double last = number(obj["last"]);

        // Coinbase does not provide the change, calculate it from the 24h open price
        const double change_percent = open > 0 ? (last - open) / open * 100 : 0;

        return Ticker::make(last, number(obj["high"]), number(obj["low"]), change_percent);
    });
}
