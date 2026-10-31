#include "aboutqt.h"
#include "ui_aboutqt.h"

AboutQt::AboutQt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutQt)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    ui->windowName->setText(this->windowTitle() + " — " + SOFT_NAME + " " + SOFT_VERSION);

    // Built from the Qt version the program runs with, so it never gets outdated
    ui->textBrowser->setOpenExternalLinks(true);
    ui->textBrowser->setHtml(QString(
        "<body style=\"font-family:'Segoe UI'; font-size:9pt;\">"
        "<p style=\"font-size:10pt; font-weight:700;\">About Qt</p>"
        "<p>This program uses Qt version %1.</p>"
        "<p>Qt is a C++ toolkit for cross-platform application development.</p>"
        "<p>Qt provides single-source portability across all major desktop operating systems. "
        "It is also available for embedded Linux and other embedded and mobile operating systems.</p>"
        "<p>Qt is available under multiple licensing options designed to accommodate the needs of our various users.</p>"
        "<p>Qt licensed under our commercial license agreement is appropriate for development of "
        "proprietary/commercial software where you do not want to share any source code with "
        "third parties or otherwise cannot comply with the terms of GNU (L)GPL.</p>"
        "<p>Qt licensed under GNU (L)GPL is appropriate for the development of Qt applications "
        "provided you can comply with the terms and conditions of the respective licenses.</p>"
        "<p>Please see <a style=\"color:#B8B857;\" href=\"https://www.qt.io/licensing/\">qt.io/licensing</a> "
        "for an overview of Qt licensing.</p>"
        "<p>Copyright (C) The Qt Company Ltd. and other contributors.</p>"
        "<p>Qt and the Qt logo are trademarks of The Qt Company Ltd.</p>"
        "<p>Qt is The Qt Company Ltd. product developed as an open source project. "
        "See <a style=\"color:#B8B857;\" href=\"https://www.qt.io/\">qt.io</a> for more information.</p>"
        "</body>").arg(qVersion()));

    // Tracking for MouseEvents
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

AboutQt::~AboutQt()
{
    delete ui;
}

void AboutQt::on_closeButton_clicked()
{
    this->close();
}

