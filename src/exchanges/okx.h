#ifndef OKX_H
#define OKX_H

#include "exchange.h"

#include <QHash>
#include <QJsonObject>

// www.okx.com (API v5) : GET /api/v5/market/tickers (24h tickers of all spot pairs in one request)
class OkxExchange : public Exchange
{
public:
    using Exchange::Exchange;

    void load_markets(MarketsCallback callback) override;

protected:
    void do_fetch(const QStringList &pairs) override;

private:
    // Calls done(tickers, ok) with { "BTC-USDT": { "last", "open24h", ... } }
    void get_tickers(std::function<void(const QHash<QString, QJsonObject> &tickers, bool ok)> done);
};

#endif // OKX_H
