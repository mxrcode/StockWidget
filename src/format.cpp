#include "format.h"

#include <QRegularExpression>
#include <QSet>
#include <cmath>

QString format_number(double value, int decimals)
{
    if (!std::isfinite(value)) return "-";

    const double magnitude = std::abs(value);

    if (decimals < 0) {
        if (magnitude >= 10) {
            decimals = 2;
        } else if (magnitude >= 1) {
            decimals = 4;
        } else {
            decimals = 6;
        }
    }

    const QString digits = QString::number(magnitude, 'f', decimals);
    const int dot = digits.indexOf('.');

    QString int_part = dot == -1 ? digits : digits.left(dot);
    QString frac_part = dot == -1 ? QString() : digits.mid(dot + 1);

    // Remove insignificant zeros, but keep at least two decimals
    while (frac_part.size() > 2 && frac_part.endsWith('0')) {
        frac_part.chop(1);
    }

    // Thousands separator
    for (int i = int_part.size() - 3; i > 0; i -= 3) {
        int_part.insert(i, ' ');
    }

    QString result = int_part;
    if (!frac_part.isEmpty()) result += "." + frac_part;

    // No sign for values that were rounded to zero
    const bool is_zero = !result.contains(QRegularExpression("[1-9]"));
    if (value < 0 && !is_zero) result.prepend('-');

    return result;
}

QString currency_prefix(const QString &quote)
{
    static const QSet<QString> dollars = {"USD", "USDT", "USDC", "BUSD", "FDUSD", "TUSD", "USDP", "DAI", "PYUSD"};

    if (dollars.contains(quote)) return "$";
    if (quote == "EUR") return "€";
    if (quote == "GBP") return "£";

    return QString();
}
