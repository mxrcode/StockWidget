#ifndef TICKER_H
#define TICKER_H

#include <QHash>
#include <QString>

// 24h statistics of one trading pair
struct Ticker
{
    enum class Status { Ok, NetworkError, UnknownPair };

    Status status = Status::NetworkError;
    double last = 0;
    double high = 0;
    double low = 0;
    double change_percent = 0;

    static Ticker make(double last, double high, double low, double change_percent) {
        return {Status::Ok, last, high, low, change_percent};
    }
    static Ticker network_error() { return {}; }
    static Ticker unknown_pair() { return {Status::UnknownPair}; }
};

// Key : trading pair as written in the config ("BTC-USDT")
using TickerMap = QHash<QString, Ticker>;

#endif // TICKER_H
