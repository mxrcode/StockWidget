#ifndef KRAKEN_H
#define KRAKEN_H

#include "exchange.h"

#include <QHash>

// www.kraken.com : GET /0/public/Ticker
// Kraken uses its own pair names ("XXBTZUSD" for BTC-USD), so the list of
// asset pairs is loaded once to translate them.
class KrakenExchange : public Exchange
{
public:
    using Exchange::Exchange;

protected:
    void do_fetch(const QStringList &pairs) override;

private:
    void load_asset_pairs(const QStringList &pairs);
    void fetch_tickers(const QStringList &pairs);

    static QString asset_name(const QString &kraken_asset);

    QHash<QString, QString> m_kraken_names; // "BTC-USD" -> "XXBTZUSD"
};

#endif // KRAKEN_H
