#include "logger.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextStream>

static QString g_log_path;
static QMutex g_log_mutex;

static constexpr qint64 kMaxLogSize = 1024 * 1024;

static void rotate_if_too_big()
{
    if (QFile(g_log_path).size() > kMaxLogSize) {
        QFile::remove(g_log_path + ".old");
        QFile::rename(g_log_path, g_log_path + ".old");
    }
}

static void message_handler(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    QMutexLocker locker(&g_log_mutex);

    // The widget may run for weeks
    rotate_if_too_big();

    QFile log_file(g_log_path);
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&log_file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " : " << msg << Qt::endl;
    }
}

void install_file_logger(const QString &path)
{
    g_log_path = path;
    qInstallMessageHandler(message_handler);
}
