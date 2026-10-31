#ifndef FORMAT_H
#define FORMAT_H

#include <QString>

// "12345.678" -> "12 345.68". With decimals < 0 the precision depends on the magnitude.
QString format_number(double value, int decimals = -1);

// Sign shown before prices for the quote currency of a pair ("USDT" -> "$")
QString currency_prefix(const QString &quote);

#endif // FORMAT_H
