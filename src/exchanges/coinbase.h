#ifndef COINBASE_H
#define COINBASE_H

#include "exchange.h"

// www.coinbase.com : GET /products/stats (24h stats of all pairs in one request)
class CoinbaseExchange : public Exchange
{
public:
    using Exchange::Exchange;

    void load_markets(MarketsCallback callback) override;

protected:
    void do_fetch(const QStringList &pairs) override;
};

#endif // COINBASE_H
