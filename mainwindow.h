#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "version.h"

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QApplication>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QFile>
#include <QSize>
#include <QPainter>
#include <QSize>
#include <QTextOption>
#include <QPointF>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDir>
#include <QPixmap>
#include <QIcon>
#include <QProcess>
#include <QDialog>
#include <QMessageBox>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QtConcurrent>
#include <QCryptographicHash>
#include <QStandardPaths>

#include "qvlabel.hpp"

#include "configurator.h"
#include "aboutqt.h"
#include "aboutme.h"
#include "warningui.h"

#include <Windows.h>
#include <tchar.h>
#include <string>

#include <shlobj.h>

#include <unzip.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *ui;
    void icon_taskbar_hider();
};

#endif // MAINWINDOW_H
