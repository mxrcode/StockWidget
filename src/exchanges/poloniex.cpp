#include "poloniex.h"

#include <QJsonObject>

void PoloniexExchange::do_fetch(const QStringList &pairs)
{
    get_each(pairs, [](const QString &pair) {
        QString symbol = pair;
        return QUrl("https://api.poloniex.com/markets/" + symbol.replace('-', '_') + "/ticker24h");
    }, [](const QString &pair, const Response &response) {
        if (!split_pair(pair, nullptr, nullptr) || response.http_status == 400) return Ticker::unknown_pair();
        if (!response.ok()) return Ticker::network_error();

        const QJsonObject obj = response.json().object();

        // "dailyChange" is a fraction : 0.0148 = 1.48%
        return Ticker::make(number(obj["close"]), number(obj["high"]), number(obj["low"]), number(obj["dailyChange"]) * 100);
    });
}
