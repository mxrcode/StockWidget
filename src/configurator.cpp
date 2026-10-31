#include "configurator.h"
#include "ui_configurator.h"

#include "exchanges/exchange.h"

#include <QColorDialog>
#include <QGraphicsOpacityEffect>
#include <QRegularExpression>

// Text color is always drawn with this opacity
static const QString kTextOpacity = "0.65";

// "color: rgba(255, 255, 255, 0.65);" -> QColor(255, 255, 255)
static QColor color_from_style(const QString &style)
{
    static const QRegularExpression regex("rgba?\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)");

    const QRegularExpressionMatch match = regex.match(style);
    if (!match.hasMatch()) return QColor(Qt::white);

    return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
}

Configurator::Configurator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Configurator)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    ui->windowName->setText(this->windowTitle());

    // Disable WebSocket RadioButton
    ui->protocol_ws_radioButton->setDisabled(1);
    QGraphicsOpacityEffect *protocol_ws_radioButton_effect = new QGraphicsOpacityEffect(this);
    protocol_ws_radioButton_effect->setOpacity(0.5);
    ui->protocol_ws_radioButton->setGraphicsEffect(protocol_ws_radioButton_effect);
    ui->protocol_https_radioButton->setChecked(true);

    // Tracking for MouseEvents
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    for (const auto &source : Exchange::available()) {
        ui->dataSources_comboBox->addItem(source.second, source.first);
    }

    ui->dataSources_image_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_color_label_style = ui->currentColor_label->styleSheet();
}

Configurator::~Configurator()
{
    delete ui;
}

void Configurator::set_config(const Config &config)
{
    m_config = config;

    ui->symbol_textEdit->setPlainText(config.pairs.join('\n'));

    // Show widget position on RadioButton
    if (config.position == "0,0") {
        ui->widgetPosition_radioButton__1->setChecked(true);
    } else if (config.position == "0,1") {
        ui->widgetPosition_radioButton__2->setChecked(true);
    } else if (config.position == "1,1") {
        ui->widgetPosition_radioButton__4->setChecked(true);
    } else { // 1,0 : Default
        ui->widgetPosition_radioButton__3->setChecked(true);
    }

    show_color(color_from_style(config.text_style));

    const int source_index = ui->dataSources_comboBox->findData(config.data_source);
    ui->dataSources_comboBox->setCurrentIndex(source_index == -1 ? 0 : source_index);

    ui->always_on_top_checkBox->setChecked(config.always_on_top);
    ui->error_notifications_checkBox->setChecked(config.error_notifications);
}

void Configurator::show_color(const QColor &color)
{
    ui->currentColor_label->setStyleSheet(m_color_label_style + "background-color: " + color.name() + ";");
}

void Configurator::on_closeButton_clicked()
{
    this->close();
}

void Configurator::on_cancelButton_clicked()
{
    this->close();
}

void Configurator::on_applyButton_clicked()
{
    Config config = m_config;

    config.pairs = Config::normalize_pairs(ui->symbol_textEdit->toPlainText().split('\n'));

    if (ui->widgetPosition_radioButton__1->isChecked()) {
        config.position = "0,0";
    } else if (ui->widgetPosition_radioButton__2->isChecked()) {
        config.position = "0,1";
    } else if (ui->widgetPosition_radioButton__4->isChecked()) {
        config.position = "1,1";
    } else {
        config.position = "1,0";
    }

    config.data_source = ui->dataSources_comboBox->currentData().toString();
    config.always_on_top = ui->always_on_top_checkBox->isChecked();
    config.error_notifications = ui->error_notifications_checkBox->isChecked();

    this->close();
    emit applied(config);
}

void Configurator::on_selectColor_button_clicked()
{
    const QColor color = QColorDialog::getColor(color_from_style(m_config.text_style), nullptr, "Select Color");

    // Check if the user selected a color
    if (color.isValid()) {
        m_config.text_style = "color: rgba(" + QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue()) + "," + kTextOpacity + ");";
        show_color(color);
    }
}
