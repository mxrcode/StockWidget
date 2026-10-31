#ifndef BYBIT_H
#define BYBIT_H

#include "exchange.h"

#include <QJsonArray>

// www.bybit.com (API v5) : GET /v5/market/tickers?category=spot (24h tickers of all spot pairs in one request)
class BybitExchange : public Exchange
{
public:
    using Exchange::Exchange;

    void load_markets(MarketsCallback callback) override;

protected:
    void do_fetch(const QStringList &pairs) override;

private:
    static QString symbol_of(const QString &pair);
    static Ticker parse_ticker(const QJsonObject &obj);

    // { "retCode": 0, "result": { "list": [...] } }, empty list and false on errors
    static QJsonArray result_list(const Response &response, bool *ok);
};

#endif // BYBIT_H
