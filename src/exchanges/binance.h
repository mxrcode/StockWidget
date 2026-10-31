#ifndef BINANCE_H
#define BINANCE_H

#include "exchange.h"

// www.binance.com & www.binance.us : GET /api/v3/ticker/24hr
class BinanceExchange : public Exchange
{
public:
    BinanceExchange(const QString &domain, QNetworkAccessManager *network, QObject *parent = nullptr);

    void load_markets(MarketsCallback callback) override;

protected:
    void do_fetch(const QStringList &pairs) override;

private:
    static QString symbol_of(const QString &pair);
    static Ticker parse_ticker(const QJsonObject &obj);

    void fetch_one_by_one(const QStringList &pairs);

    QString m_domain;
};

#endif // BINANCE_H
