#include "config.h"
#include "version.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>

// Pair lines hidden from the widget : a comment for older versions
static const QString kHiddenPrefix = "#off:";

QStringList Config::shown_pairs() const
{
    QStringList shown;
    for (const QString &key : pairs) {
        if (!hidden_pairs.contains(key)) shown.append(key);
    }
    return shown;
}

QString Config::file_path()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(CONFIG_NAME);
}

Config Config::load()
{
    Config config;

    QFile file(file_path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const char *pair : {"BTC-USDT", "ETH-USDT", "LTC-USDT", "BNB-USDT", "CAKE-USDT"}) {
            config.pairs.append(pair_key(config.data_source, QString::fromLatin1(pair)));
        }
        if (!config.save()) qInfo() << "Could not create" << file_path();
        return config;
    }

    QList<QPair<QString, bool>> pair_lines; // {line, hidden}
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.startsWith(kHiddenPrefix)) {
            pair_lines.append({line.mid(kHiddenPrefix.size()), true});
            continue;
        }
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) continue;

        if (!line.startsWith("$")) {
            pair_lines.append({line, false});
            continue;
        }

        const int colon = line.indexOf(':');
        if (colon == -1) continue;

        const QString key = line.mid(1, colon - 1);
        QString value = line.mid(colon + 1);

        // Delete comments after "//"
        const int comment = value.indexOf("//");
        if (comment != -1) value = value.left(comment);
        value = value.trimmed();

        if (key == "text_style") {
            config.text_style = value;
        } else if (key == "position") {
            config.position = value;
        } else if (key == "always_on_top") {
            config.always_on_top = value.toInt();
        } else if (key == "data_sources") {
            config.data_source = value;
        } else if (key == "error_notifications") {
            config.error_notifications = value.toInt();
        } else if (key == "color_changes") {
            config.color_changes = value.toInt();
        } else if (key == "refresh_interval") {
            config.refresh_interval = qBound(kMinRefreshInterval, value.toInt(), kMaxRefreshInterval);
        }
        // Other parameters (e.g. the removed $auto_update) are skipped
    }

    // Pairs are resolved at the end : $data_sources may follow them
    for (const auto &[line, hidden] : pair_lines) {
        const int colon = line.indexOf(':');
        const QString source = colon == -1 ? config.data_source : line.left(colon).trimmed().toLower();
        const QString pair = normalize_pair(colon == -1 ? line : line.mid(colon + 1));

        const QString key = pair_key(source, pair);
        if (pair.isEmpty() || config.pairs.contains(key)) continue;

        config.pairs.append(key);
        if (hidden) config.hidden_pairs.insert(key);
    }

    return config;
}

bool Config::save() const
{
    QSaveFile file(file_path());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream fout(&file);
    fout << "// " << SOFT_NAME << " " << SOFT_VERSION << " : One line - One trading pair (\"exchange:PAIR\" for a non-default exchange, \"" << kHiddenPrefix << "PAIR\" to hide it)." << Qt::endl;
    fout << Qt::endl;
    fout << "$text_style:" << text_style << Qt::endl;
    fout << "$position:" << position << Qt::endl;
    fout << "$always_on_top:" << int(always_on_top) << Qt::endl;
    fout << "$data_sources:" << data_source << Qt::endl;
    fout << "$error_notifications:" << int(error_notifications) << Qt::endl;
    fout << "$color_changes:" << int(color_changes) << Qt::endl;
    fout << "$refresh_interval:" << refresh_interval << Qt::endl;
    fout << Qt::endl;

    for (const QString &key : pairs) {
        if (hidden_pairs.contains(key)) fout << kHiddenPrefix;
        fout << (source_of(key) == data_source ? pair_of(key) : key) << Qt::endl;
    }

    fout.flush();
    return file.commit();
}

QString Config::normalize_pair(QString pair)
{
    pair = pair.trimmed().toUpper();
    pair.replace('_', '-');
    pair.replace('/', '-');
    pair.replace(' ', '-');
    return pair;
}

QString Config::pair_key(const QString &source, const QString &pair)
{
    return source + ":" + pair;
}

QString Config::source_of(const QString &key)
{
    return key.section(':', 0, 0);
}

QString Config::pair_of(const QString &key)
{
    return key.section(':', 1);
}

bool Config::is_free_position(const QString &position)
{
    return position.startsWith('@');
}

QPoint Config::free_position(const QString &position)
{
    const QStringList xy = position.mid(1).split(',');
    if (xy.size() != 2) return QPoint();

    return QPoint(xy[0].trimmed().toInt(), xy[1].trimmed().toInt());
}

QString Config::make_free_position(const QPoint &point)
{
    return QString("@%1,%2").arg(point.x()).arg(point.y());
}
