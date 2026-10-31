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

    void load_markets(MarketsCallback callback) override;

protected:
    void do_fetch(const QStringList &pairs) override;

private:
    // Calls done(ok) when m_kraken_names is filled
    void load_asset_pairs(std::function<void(bool ok)> done);
    void fetch_tickers(const QStringList &pairs);

    static QString asset_name(const QString &kraken_asset);
    static Ticker parse_ticker(const QJsonObject &data);

    QHash<QString, QString> m_kraken_names; // "BTC-USD" -> "XXBTZUSD"
};

#endif // KRAKEN_H
