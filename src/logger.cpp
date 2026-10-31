#include "logger.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextStream>

static QString g_log_path;
static QMutex g_log_mutex;

static constexpr qint64 kMaxLogSize = 1024 * 1024;

static void message_handler(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    QMutexLocker locker(&g_log_mutex);

    QFile log_file(g_log_path);
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&log_file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " : " << msg << Qt::endl;
    }
}

void install_file_logger(const QString &path)
{
    g_log_path = path;

    if (QFile(path).size() > kMaxLogSize) {
        QFile::remove(path + ".old");
        QFile::rename(path, path + ".old");
    }

    qInstallMessageHandler(message_handler);
}
