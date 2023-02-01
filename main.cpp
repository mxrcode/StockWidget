#include "mainwindow.h"
#include "ui_mainwindow.h"

QString digit_format (QString number, int decimal_places = 2) {

    QStringList parts = number.split(".");
    QString decimalPart = parts[1];
    decimalPart.chop(decimalPart.size() - decimal_places);
    number = parts[0] + "." + decimalPart;

    // Insert a space every 3 digits
    int dotIndex = number.indexOf(".");
    if (dotIndex == -1) return number; // number doesn't contain a dot

    for (int i = dotIndex - 3; i > 0; i -= 3) {
        number.insert(i, ' ');
    }

    // Trim trailing zeros after dot
    while (number.endsWith("0") && number.at(dotIndex + 1) != '\0') {
        if (number.at(number.size()-3) == '.') break;
        number.chop(1);
    }
    if (number.endsWith(".")) number.chop(1);

    return number;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create a tray icon and a menu
    QSystemTrayIcon trayIcon;
    QMenu trayMenu;

    // Remove the empty filed for Icons
    trayMenu.setStyleSheet("QMenu::item::icon{width: 0px;} QMenu::item::icon:disabled {width: 0px;} QMenu::item::icon:off {width: 0px;} QMenu::item::icon:on {width: 0px;} QMenu::item::icon:selected {width: 0px;} QMenu::item::icon:checked {width: 0px;} QMenu::indicator { width: 0px;}");

    // Add a "Config" action to the menu
    Configurator configurator;
    QAction *configuratorAction = trayMenu.addAction("Edit Config");
    QObject::connect(configuratorAction, &QAction::triggered, &app, [&]() {

        configurator.show();

    });

    // Add a "Edit Config" action to the menu
//    QAction *editAction = trayMenu.addAction("Edit Config");
//    QObject::connect(editAction, &QAction::triggered, &app, [&]() {
//        QProcess::startDetached("notepad.exe", {CONFIG_NAME});
//    });

    // Add a "Restart App" action to the menu
    QAction *restartAction = trayMenu.addAction("Restart App");
    QObject::connect(restartAction, &QAction::triggered, &app, [&]() {
        // Start a new instance of the application
        QProcess::startDetached(qApp->applicationFilePath());

        // Close the current instance
        qApp->quit();
    });

    // Separator
    QAction* separator_1 = new QAction();
    separator_1->setSeparator(true);
    trayMenu.addAction(separator_1);

    // Add an "About Qt" action to the menu
    AboutMe AboutMe;
    QAction *aboutMeAction = trayMenu.addAction("About Me");
    QObject::connect(aboutMeAction, &QAction::triggered, qApp, [&](){
        AboutMe.show();
    });

    // Add an "About Qt" action to the menu
    AboutQt AboutQt;
    QAction *aboutQtAction = trayMenu.addAction("About Qt");
    QObject::connect(aboutQtAction, &QAction::triggered, qApp, [&](){
        AboutQt.show();
//        QMessageBox aboutQtBox;
//        aboutQtBox.setWindowIcon(QPixmap(":/img/icon-bg.svg"));
//        aboutQtBox.setWindowTitle("About Qt — " + SOFT_NAME + " " + SOFT_VERSION);
//        QMessageBox::aboutQt(&aboutQtBox);
    });

    // Separator
    QAction* separator_2 = new QAction();
    separator_2->setSeparator(true);
    trayMenu.addAction(separator_2);

    // Add a "Close" action to the menu
    QAction *closeAction = trayMenu.addAction("Close");
    QObject::connect(closeAction, &QAction::triggered, &app, &QApplication::quit);

    // Set the tray icon and menu
    trayIcon.setContextMenu(&trayMenu);

    QString pixmapPath = ":/img/icon.svg";
    QPixmap pixmap(pixmapPath);
    QIcon icon(pixmap);
    trayIcon.setIcon(icon);

    trayIcon.show();

    QString text_style = "color:rgba(255, 255, 255, 0.65);";
    QString widget_position = "1,0";

    bool auto_update = 1;

    // DATA
    QVector<QString> symbol_list;
    QFile file(CONFIG_NAME);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString tmp = stream.readLine();
            if (tmp.isEmpty() || tmp.startsWith("//") || tmp.startsWith("#")) continue;
            if (tmp.startsWith("$text_style:")) {

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$text_style:", "");

                text_style = tmp;
                continue;
            }
            if (tmp.startsWith("$position:")) {

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$position:", "");

                widget_position = tmp;

                continue;
            }
            if (tmp.startsWith("$auto_update:")) {

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$auto_update:", "");

                auto_update = tmp.toInt();

                continue;
            }

            symbol_list.append(tmp);
        }
        file.close();
    } else {
        symbol_list = {
            "BTCUSDT",
            "ETHUSDT",
            "LTCUSDT",
            "BNBUSDT",
            "CAKEUSDT",
        };

        // Open the file for writing.
        if (file.open(QFile::WriteOnly | QFile::Text)) {
          // The file was opened successfully.
          QTextStream fout(&file);

          // Write default config to the file.
          fout << "// " << SOFT_NAME << " " << SOFT_VERSION << " : One line - One trading pair." << Qt::endl;
          fout << "// We use the Binance Api to get the exchange rate and other data on trading pairs." << Qt::endl;
          fout << Qt::endl;
          fout << "$text_style:color:rgba(255, 255, 255, 0.65);" << Qt::endl;
          fout << "$position:" << widget_position << Qt::endl;
          fout << "$auto_update:" << "1" << Qt::endl;
          fout << Qt::endl;

          for (QString symbol : symbol_list) {
            fout << symbol << Qt::endl;
          }

          // Close the file.
          file.close();
        } else {
            return -1;
        }
    }

    // Create the main window
    MainWindow mainWindow;
    Ui::MainWindow *ui = mainWindow.ui;
    mainWindow.show();

    const int mainWindow_width = 420;

    // FIND SCREEN SIZE & MOVE WINDOW
    QScreen * screen_one = QApplication::screens().at(0);
    QSize screen_size = screen_one->availableSize();
    mainWindow.setGeometry(0, 0, mainWindow_width, (symbol_list.count()*50)+10);
    mainWindow.setWindowTitle(SOFT_NAME + " " + SOFT_VERSION);

    // About : widget_position
    // ⌜¯¯¯¯¯¯¯¯¯¯¯¯⌝
    // | 0,0    0,1 |
    // |            |
    // | D1,0   1,1 |
    // ⌞____________⌟

    if (widget_position == "1,0") {
        mainWindow.move(0, ( screen_size.height() - ( (symbol_list.count()*50)+10 ) ));
    } else if (widget_position == "0,0") {
        mainWindow.move(0, 0);
    } else if (widget_position == "0,1") {
        mainWindow.move((screen_size.width() - (mainWindow_width-36)), 0);
    } else if (widget_position == "1,1") {
        mainWindow.move((screen_size.width() - (mainWindow_width-36)), ( screen_size.height() - ( (symbol_list.count()*50)+10 ) ));
    } else {
        mainWindow.move(0, ( screen_size.height() - ( (symbol_list.count()*50)+10 ) ));
    }

    // Create a vector of QLabels
    QMap<QString, QVector<QLabel*>> labels_map;
    labels_map.insert("symbol", {});
    labels_map.insert("price", {});
    labels_map.insert("priceHighLow", {});
    labels_map.insert("pricechangepercent", {});

    for (QString label_list : labels_map.keys()) {
        // Create some labels and add them to the vector
        for (int i = 0; i < symbol_list.size(); i++) {
            QLabel *label = new QLabel;
            label->setText("Loading...");

            QFont font("Roboto", 14, QFont::Normal);
            font.setStyleStrategy(QFont::PreferAntialias);
            label->setFont(font);
            label->setStyleSheet(text_style);

            labels_map[label_list].append(label);
        }

        // Display the labels : add in layouts
        if (label_list == "price") {
            for (QLabel *label : labels_map[label_list]) {
                ui->verticalLayout_price->addWidget(label);
            }
        } else if (label_list == "symbol") {
            for (QLabel *label : labels_map[label_list]) {
                ui->verticalLayout_symbol->addWidget(label);
            }
        } else if (label_list == "priceHighLow") {
            for (QLabel *label : labels_map[label_list]) {
                ui->verticalLayout_priceHighLow->addWidget(label);
            }
        } else if (label_list == "pricechangepercent") {
            for (QLabel *label : labels_map[label_list]) {
                ui->verticalLayout_priceChangePercent->addWidget(label);
            }
        }
    }

    QVector<QVLabel*> hl_difference;
    // Create some QVLabels and add them to the vector
    for (int i = 0; i < symbol_list.size(); i++) {
        QVLabel *qvlabel = new QVLabel;
        qvlabel->setText("NTWRK");
        qvlabel->resize(50, 50);
        qvlabel->rotate(270);
        qvlabel->setXYWH(-48, -20, 50, 50);

        QFont font("Roboto", 10, QFont::Normal);
        font.setStyleStrategy(QFont::PreferAntialias);
        qvlabel->setFont(font);
        qvlabel->setStyleSheet(text_style);

        // Add the widget to the main window layout
        ui->verticalLayout_difference->addWidget(qvlabel);

        hl_difference.push_back(qvlabel);
    }

    // Create a QTimer
    QTimer *timer = new QTimer;
    timer->setInterval(100); // update every X ms
    timer->start();

    // Create a counter variable
    int i_connect = 0;

    // Connect the timeout signal to a lambda function
    QObject::connect(timer, &QTimer::timeout, [&]() {

        if (i_connect == 0) timer->setInterval(60*1000); // update every X ms

        QJsonArray symbols_json;
        for (const auto &symbol : symbol_list) {
            symbols_json.append(symbol);
        }
        QJsonDocument jsonDoc(symbols_json);
        QString json_string = jsonDoc.toJson(QJsonDocument::Compact);

        // Create a network manager.
        QNetworkAccessManager manager;

        QString binance_domain;
        QString option_binance_domain = "global"; // This line exists because the regular Binance API doesn't work in the US

        if (option_binance_domain == "global") {
            binance_domain = "api.binance.com";
        } else if (option_binance_domain == "us") {
            binance_domain = "api.binance.us";
        }

        QNetworkRequest request(QUrl("https://" + binance_domain + "/api/v3/ticker/24hr?symbols=" + json_string));
        auto *reply = manager.get(request);

        // Wait for the request to complete
        QEventLoop eventLoop;
        QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        // Get the response data.
        QByteArray data = reply->readAll();

        // Parse the JSON data
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray json_array = doc.array();

        bool net_status = (json_array.empty()) ? 1 : 0;

        for (int i = 0; i < symbol_list.size(); ++i) {

            if (net_status == 1) {

                labels_map["symbol"].at(i)->setText("net");
                labels_map["price"].at(i)->setText("net");
                labels_map["priceHighLow"].at(i)->setText("net");
                labels_map["pricechangepercent"].at(i)->setText("net");
                hl_difference.at(i)->setText("net");
                hl_difference.at(i)->repaint();

                net_status = 1;
            }

            if (net_status == 0) {

                QJsonValue values = json_array[i];
                QJsonObject obj = values.toObject();

                QString symbol = obj["symbol"].toString();
                QString priceValue = obj["lastPrice"].toString();
                QString priceHigh24h = obj["highPrice"].toString();
                QString priceLow24h = obj["lowPrice"].toString();
                QString priceChangePercent = obj["priceChangePercent"].toString();

                // Set symbol
                QFont fontRoboto_18("Roboto", 18, QFont::Normal);
                fontRoboto_18.setStyleStrategy(QFont::PreferAntialias);
                labels_map["symbol"].at(i)->setFont(fontRoboto_18);
                labels_map["symbol"].at(i)->setText(symbol.replace("USDT", ""));

                // Set coin price
                int priceValue_decimal;

                if ( priceValue.toDouble() <= 1.00 ) {
                    priceValue_decimal = 6;
                } else if ( priceValue.toDouble() <= 10.00 ) {
                    priceValue_decimal = 4;
                } else {
                    priceValue_decimal = 2;
                }

                labels_map["price"].at(i)->setText("$" + digit_format(priceValue, priceValue_decimal));

                // Set coin High & Low price
                QFont fontRoboto_8("Roboto", 8, QFont::Normal);
                fontRoboto_8.setStyleStrategy(QFont::PreferAntialias);
                labels_map["priceHighLow"].at(i)->setFont(fontRoboto_8);

                int priceHigh24h_decimal;

                if ( priceHigh24h.toDouble() <= 1.00 ) {
                    priceHigh24h_decimal = 6;
                } else if ( priceHigh24h.toDouble() <= 10.00 ) {
                    priceHigh24h_decimal = 4;
                } else {
                    priceHigh24h_decimal = 2;
                }

                labels_map["priceHighLow"].at(i)->setText("$" + digit_format(priceHigh24h, priceHigh24h_decimal) + "<br>" + "$" + digit_format(priceLow24h, priceHigh24h_decimal));

                // Set coin price change percent
                labels_map["pricechangepercent"].at(i)->setText("%" + digit_format(priceChangePercent));

                // Set coin High/Low price difference
                double dH24h = priceHigh24h.toDouble();
                double dL24h = priceLow24h.toDouble();
                hl_difference.at(i)->setText("%" + digit_format( QString::number((dH24h-dL24h)/(dL24h/100))));
                hl_difference.at(i)->repaint();

            }
        } net_status = 0;

        i_connect = 1;
    });

    return app.exec();
}
