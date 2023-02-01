#include "warningui.h"
#include "ui_warningui.h"

WarningUi::WarningUi(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WarningUi)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    ui->windowName->setText(this->windowTitle());

    // Tracking for MouseEvents
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

WarningUi::~WarningUi()
{
    delete ui;
}

void WarningUi::on_closeButton_clicked()
{
    this->close();
}

void WarningUi::on_OkButton_clicked()
{
    this->close();
}

void WarningUi::setTextBrowser(QString text)
{
    ui->textBrowser->setText(text);
}
