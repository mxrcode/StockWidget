#ifndef POLONIEX_H
#define POLONIEX_H

#include "exchange.h"

// poloniex.com (API v3) : GET /markets/{BASE_QUOTE}/ticker24h, one request per pair
class PoloniexExchange : public Exchange
{
public:
    using Exchange::Exchange;

protected:
    void do_fetch(const QStringList &pairs) override;
};

#endif // POLONIEX_H
