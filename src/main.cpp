#include "app.h"
#include "logger.h"
#include "version.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFontDatabase>
#include <QLockFile>
#include <QProcess>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(SOFT_NAME);
    app.setApplicationVersion(SOFT_VERSION);

    // The widget lives in the tray, closing a dialog must not quit it
    app.setQuitOnLastWindowClosed(false);

    install_file_logger(QDir(QCoreApplication::applicationDirPath()).filePath("qInfo.log"));

    // Only one widget per user session (autorun + manual start)
    QLockFile lock(QDir::temp().filePath(SOFT_NAME + ".lock"));
    if (!lock.tryLock(3000)) {
        qInfo() << "Another instance is already running";
        return 0;
    }

    if (QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf") == -1) {
        qInfo() << "Error loading font \"Roboto-Regular.ttf\"";
    }

    int result = 0;
    bool restart = false;
    {
        App stock_widget;
        stock_widget.start();

        result = app.exec();
        restart = stock_widget.restart_requested();
    }

    if (restart) {
        lock.unlock();
        QProcess::startDetached(QCoreApplication::applicationFilePath());
    }

    return result;
}
