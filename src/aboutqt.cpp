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

