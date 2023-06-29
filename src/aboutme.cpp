#include "aboutme.h"
#include "ui_aboutme.h"

AboutMe::AboutMe(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutMe)
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

AboutMe::~AboutMe()
{
    delete ui;
}

void AboutMe::on_closeButton_clicked()
{
    this->close();
}
