#ifndef COINBASE_H
#define COINBASE_H

#include "exchange.h"

// www.coinbase.com : GET /products/{BASE-QUOTE}/stats, one request per pair
class CoinbaseExchange : public Exchange
{
public:
    using Exchange::Exchange;

protected:
    void do_fetch(const QStringList &pairs) override;
};

#endif // COINBASE_H
