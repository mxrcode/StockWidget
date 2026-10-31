#include "config.h"
#include "version.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>

QString Config::file_path()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(CONFIG_NAME);
}

Config Config::load()
{
    Config config;

    QFile file(file_path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        config.pairs = {"BTC-USDT", "ETH-USDT", "LTC-USDT", "BNB-USDT", "CAKE-USDT"};
        if (!config.save()) qInfo() << "Could not create" << file_path();
        return config;
    }

    QStringList pairs;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) continue;

        if (!line.startsWith("$")) {
            pairs.append(line);
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
        }
        // Other parameters (e.g. the removed $auto_update) are skipped
    }

    config.pairs = normalize_pairs(pairs);
    return config;
}

bool Config::save() const
{
    QSaveFile file(file_path());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream fout(&file);
    fout << "// " << SOFT_NAME << " " << SOFT_VERSION << " : One line - One trading pair." << Qt::endl;
    fout << Qt::endl;
    fout << "$text_style:" << text_style << Qt::endl;
    fout << "$position:" << position << Qt::endl;
    fout << "$always_on_top:" << int(always_on_top) << Qt::endl;
    fout << "$data_sources:" << data_source << Qt::endl;
    fout << "$error_notifications:" << int(error_notifications) << Qt::endl;
    fout << Qt::endl;

    for (const QString &pair : pairs) {
        fout << pair << Qt::endl;
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

QStringList Config::normalize_pairs(const QStringList &pairs)
{
    QStringList result;
    for (const QString &raw : pairs) {
        const QString pair = normalize_pair(raw);
        if (!pair.isEmpty() && !result.contains(pair)) result.append(pair);
    }
    return result;
}
