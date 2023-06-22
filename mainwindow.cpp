#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    icon_taskbar_hider();
}

MainWindow::~MainWindow()
{
    QThreadPool::globalInstance()->clear();
    delete ui;
}

void MainWindow::icon_taskbar_hider()
{
    // taskbar icon hide
    HWND hWnd = (HWND)winId();
    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
}
