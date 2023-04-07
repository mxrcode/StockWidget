#include "configurator.h"
#include "ui_configurator.h"

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
    QGraphicsOpacityEffect *protocol_ws_radioButton_effect = new QGraphicsOpacityEffect();
    protocol_ws_radioButton_effect->setOpacity(0.5);
    ui->protocol_ws_radioButton->setGraphicsEffect(protocol_ws_radioButton_effect);
    ui->protocol_https_radioButton->setChecked(true);

    // Tracking for MouseEvents
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Initialize the m_data_sources
    m_data_sources = {
        {"binance_com", "Binance Global"},
        {"binance_us", "Binance US"},
        {"coinbase_com", "Coinbase"},
        {"poloniex_com", "Poloniex"}
    };

    // Configurator : Get Data
    QVector<QString> symbol_list;

    QFile file(CONFIG_NAME);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString tmp = stream.readLine();
            if (tmp.isEmpty() || tmp.startsWith("//") || tmp.startsWith("#")) continue;
            if (tmp.startsWith("$text_style:")) { // TEXT_STYLE

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$text_style:", "");

                m_text_style = tmp;
                continue;
            }
            if (tmp.startsWith("$position:")) { // WIDGET POSITION

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$position:", "");

                m_widget_position = tmp;

                continue;
            }
            if (tmp.startsWith("$auto_update:")) { // AUTO-UPDATE

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$auto_update:", "");

                m_auto_update = tmp;

                continue;
            }
            if (tmp.startsWith("$always_on_top:")) { // ALWAYS ON TOP

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$always_on_top:", "");

                m_always_on_top = tmp;

                continue;
            }
            if (tmp.startsWith("$data_sources:")) { // DATA SOURCES

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$data_sources:", "");

                m_data_sources_current = tmp;

                continue;
            }
            if (tmp.startsWith("$error_notifications:")) { // ERROR NOTIFICATION

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$error_notifications:", "");

                m_error_notifications = tmp;

                continue;
            }
            if (tmp.startsWith("$")) continue; // Skip all other parameters

            symbol_list.append(tmp);
        }
        // Close the file.
        file.close();

        // Convert from QVector to simple QString
        for (int i = 0; i < symbol_list.size(); i++) {
            if (i != 0) m_symbols_string += "<br>";
            m_symbols_string += symbol_list.at(i);
        }
        ui->symbol_textEdit->setText(m_symbols_string);

        // Show widget position on RadioButton
        if (m_widget_position == "1,0") {
            ui->widgetPosition_radioButton__3->setChecked(true);
        } else if (m_widget_position == "0,0") {
            ui->widgetPosition_radioButton__1->setChecked(true);
        } else if (m_widget_position == "0,1") {
            ui->widgetPosition_radioButton__2->setChecked(true);
        } else if (m_widget_position == "1,1") {
            ui->widgetPosition_radioButton__4->setChecked(true);
        } else { // 1,0 : Default
            ui->widgetPosition_radioButton__3->setChecked(true);
        }

        QString t_text_style = m_text_style;

        t_text_style.replace("color:", "background-color:");
        t_text_style.replace("0.65", "1"); // Change Opacity for visual color accuracy
        ui->currentColor_label->setStyleSheet(ui->currentColor_label->styleSheet() + t_text_style);

        // Auto Update
        ui->auto_update_checkBox->setChecked(m_auto_update.toInt());

        warning_ui = new WarningUi;
        connect(this, &Configurator::setWarningUiText, warning_ui, &WarningUi::setTextBrowser);

        ui->error_notifications_checkBox->setChecked(m_error_notifications.toInt());
    }

    // Add items to the QComboBox (dataSources_comboBox)
    QMap<QString, QString>::iterator it;
    int it_qmap_i = 0;
    for (it = m_data_sources.begin(); it != m_data_sources.end(); ++it, ++it_qmap_i)
    {
        ui->dataSources_comboBox->addItem(it.value(), it.key());

        // Set the default selected item (dataSources_comboBox)
        if (m_data_sources_current == it.key()) ui->dataSources_comboBox->setCurrentIndex(it_qmap_i);
    } it_qmap_i = 0;

    ui->dataSources_image_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    constructor_unlocked = true;
}

Configurator::~Configurator()
{
    delete ui;
}

void Configurator::on_closeButton_clicked()
{
    this->close();
}

void Configurator::on_applyButton_clicked()
{
    QFile file(CONFIG_NAME);
    // Open the file for writing.
    if (file.open(QFile::WriteOnly | QFile::Text)) {
      // The file was opened successfully.
      QTextStream fout(&file);

      // Write default config to the file.
      fout << "// " << SOFT_NAME << " " << SOFT_VERSION << " : One line - One trading pair." << Qt::endl;
      fout << Qt::endl;
      fout << "$text_style:" << m_text_style << Qt::endl;
      fout << "$position:" << m_widget_position << Qt::endl;
      fout << "$auto_update:" << m_auto_update << Qt::endl;
      fout << "$always_on_top:" << m_always_on_top << Qt::endl;
      fout << "$data_sources:" << m_data_sources_current << Qt::endl;
      fout << "$error_notifications:" << m_error_notifications << Qt::endl;
      fout << Qt::endl;

      QVector<QString> symbol_list = ui->symbol_textEdit->toPlainText().split("\n");

      // QVector uniqueizer
      QVector<QString> t_uniqueVector;
      for (const QString &str : symbol_list) {
          if (!t_uniqueVector.contains(str)) {
              t_uniqueVector.append(str);
          }
      }
      symbol_list = t_uniqueVector;

      // toUpper
      std::for_each(symbol_list.begin(), symbol_list.end(), [](QString &s){ s = s.toUpper(); });

      // Replacing "_"(etc...) with "-" in symbol_list
      for (QString& symbol : symbol_list) {
          if (symbol.contains('_')) {
              symbol.replace('_', '-');
          }
          else if (symbol.contains(' ')) {
              symbol.replace(' ', '-');
          }
      }

      for (QString symbol : symbol_list) {
        fout << symbol << Qt::endl;
      }

      // Close the file.
      file.close();

      // Start a new instance of the application
      QProcess::startDetached(qApp->applicationFilePath());

      // Close the current instance
      qApp->quit();
    } else {
        emit setWarningUiText("Unknown error when trying to open file \"" + CONFIG_NAME + "\". Make sure the file exists and that you have permission to write to it.");
        warning_ui->show();
    }
}

void Configurator::on_cancelButton_clicked()
{
    this->close();
}

// selectColor_button
void Configurator::on_selectColor_button_clicked()
{
    // Open the QColorDialog
    QColorDialog colorDialog;
    colorDialog.setWindowTitle("Select Color");
    QColor color = colorDialog.getColor();

    // Check if the user selected a color
    if (color.isValid()) {
        // Set the color of the currentColorButton
        // ui->currentColor_label->setStyleSheet(ui->currentColor_label->styleSheet() + "background-color:" + color.name(QColor::HexArgb) + ";");
        /* FOR RGBA */QString t_styleSheet = ui->currentColor_label->styleSheet() + "background-color: rgba(" + QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue()) + "," + QString::number(color.alpha()) + ");";
        /* FOR RGBA */m_text_style = "color: rgba(" + QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue()) + "," + "0.65" + ");";
        ui->currentColor_label->setStyleSheet(t_styleSheet);
    }
}

// widgetPosition_radioButton
void Configurator::on_widgetPosition_radioButton__1_clicked() {m_widget_position = "0,0";}
void Configurator::on_widgetPosition_radioButton__2_clicked() {m_widget_position = "0,1";}
void Configurator::on_widgetPosition_radioButton__3_clicked() {m_widget_position = "1,0";}
void Configurator::on_widgetPosition_radioButton__4_clicked() {m_widget_position = "1,1";}

// Auto Update
void Configurator::on_auto_update_checkBox_stateChanged(int arg1) {m_auto_update = (ui->auto_update_checkBox->isChecked()) ? "1" : "0";}

// Data Sources
void Configurator::on_dataSources_comboBox_currentIndexChanged(int index)
{
    if (constructor_unlocked == true) m_data_sources_current = ui->dataSources_comboBox->currentData().toString();
}

// Error Notifications
void Configurator::on_error_notifications_checkBox_stateChanged(int arg1) {m_error_notifications = (ui->error_notifications_checkBox->isChecked()) ? "1" : "0";}

