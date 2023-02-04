#include "mainwindow.h"
#include "ui_mainwindow.h"

QString get_hwid() {
    DWORD serialNum = 0;
    GetVolumeInformation(_T("C:\\"), NULL, 0, &serialNum, NULL, NULL, NULL, 0);
    return QString::number(serialNum);
}

QString get_client_id() {
    return "client[" + get_hwid() + "]";
}

long version_convert(QString version) {
    QStringList parts = version.split(".");
    int major = parts.at(0).toInt();
    int minor = parts.at(1).toInt();
    int patch = parts.at(2).toInt();
    return major * 1000000 + minor * 10000 + patch * 100;
}

struct UpdateInfo {
    QString status;
    QString endpoint;
    long latest_version;
    QString latest_version_string;
    bool update_available;
    QString update_url;
    QString update_url_sha256;
    QString user_ip;
};

UpdateInfo getUpdateInfo()
{
    UpdateInfo info;

    try {

        QNetworkAccessManager *manager = new QNetworkAccessManager();
        QUrl url("https://a8de92e8b7b48f080daaf1b0900c0632.block17.icu/api/v1/getUpdate");
        QNetworkRequest request(url);
        QString user_agent = SOFT_NAME + " " + SOFT_VERSION + " " + get_client_id() + " action[get_update_info]";
        request.setRawHeader("User-Agent", user_agent.toUtf8());

        QNetworkReply *reply = manager->get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        QJsonObject jsonObj = jsonDoc.object();

        QString status = jsonObj.value("status").toString();

        if (status == "OK") {
            // Get values from JSON object
            info.status = status;
            info.endpoint = jsonObj.value("endpoint").toString();
            info.latest_version = jsonObj.value("latest_version").toInt();
            info.latest_version_string = jsonObj.value("latest_version_string").toString();
            info.update_available = (bool)jsonObj.value("update_available").toString().toInt();
            info.update_url = jsonObj.value("update_url").toString();
            info.update_url_sha256 = jsonObj.value("update_url_sha256").toString();
            info.user_ip = jsonObj.value("user-ip").toString();
        }

        reply->deleteLater();

    } catch (...) {
    }

    return info;
}

QString sha256_by_link(QString s_url) {

    QNetworkAccessManager manager;
    QUrl url(s_url);
    QNetworkRequest request(url);
    QString user_agent = SOFT_NAME + " " + SOFT_VERSION + " " + get_client_id() + " action[download_sha256]";
    request.setRawHeader("User-Agent", user_agent.toUtf8());
    QNetworkReply *reply = manager.get(request);

    // Wait for the request to complete
    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QString t_sha256 = reply->readAll();
    int length = t_sha256.indexOf(" ");
    QString sha256 = t_sha256.left(length);

    return sha256.trimmed();

}

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

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) // Part for outputting debug info to qInfo.log
{
    QFile logFile("qInfo.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        QString t_msg = QDateTime::currentDateTime().toString() + " : " + msg;
        stream << t_msg << Qt::endl;
    }
}

void file_remover (QString file_name) {
    QFile file(file_name);
    if (file.exists()) {
        bool result = file.remove();
        if (result) {
            qInfo() << "File" << file_name << "was successfully deleted.";
        } else {
            qInfo() << "Could not delete file" << file_name << ".";
        }
    } else {
        qInfo() << "File" << file_name << "does not exist.";
    }

    return;
}

bool is_soft_in_autorun()
{
    QString startup_dir = QDir::toNativeSeparators(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()) + QDir::separator() + "Startup" + QDir::separator();
    QString lnk_path = startup_dir + SOFT_NAME + ".lnk";

    QFile link(lnk_path);
    if (link.exists()) {
        return true;
    } else {
        return false;
    }
}

void add_soft_to_autorun(QApplication &app) { // Another way to create .lnk in Windows

    QString startup_dir = QDir::toNativeSeparators(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()) + QDir::separator() + "Startup" + QDir::separator();
    QString lnk_path = startup_dir + SOFT_NAME + ".lnk";

    QFile link(lnk_path);
    if (link.exists()) {
        link.remove();
    }

    IShellLink *shell_link;
    CoInitialize(NULL);
    HRESULT result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<void**>(&shell_link));
    if (result == S_OK) {
        IPersistFile *persistFile;
        shell_link->SetPath(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(app.applicationFilePath()).utf16()));
        result = shell_link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persistFile));
        if (result == S_OK) {
            persistFile->Save(reinterpret_cast<const wchar_t *>(lnk_path.utf16()), TRUE);
            persistFile->Release();
        }
        shell_link->Release();
    }
    CoUninitialize();
}

void remove_soft_from_autorun()
{
    QString startup_dir = QDir::toNativeSeparators(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()) + QDir::separator() + "Startup" + QDir::separator();
    QString lnk_path = startup_dir + SOFT_NAME + ".lnk";

    QFile link(lnk_path);
    if (link.exists()) {
        link.remove();
    }
}

/* Part of the code causing problems in work : Widget not starts automatically
bool is_soft_in_autorun()
{
    HKEY hKey;
    DWORD dwType = REG_SZ;
    TCHAR szPath[MAX_PATH];
    DWORD dwSize = sizeof(szPath);

    // Open the autorun registry key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        // Check if the program is in the autorun list
        if (RegQueryValueEx(hKey, TEXT(dSOFT_NAME), 0, &dwType, (LPBYTE)&szPath, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

void add_soft_to_autorun()
{
    HKEY hKey;
    DWORD dwSize = (wcslen(QString(qApp->applicationFilePath()).toStdWString().c_str()) + 1) * sizeof(wchar_t);

    // Open the autorun registry key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        QString qs_path = QDir::toNativeSeparators(qApp->applicationFilePath());

        // Add the program to the autorun list
        RegSetValueEx(hKey, TEXT(dSOFT_NAME), 0, REG_SZ, (LPBYTE)QString(qs_path).toStdWString().c_str(), dwSize);
        RegCloseKey(hKey);
    }
}

void remove_soft_from_autorun()
{
    HKEY hKey;

    // Open the autorun registry key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        // Remove the program from the autorun list
        RegDeleteValue(hKey, TEXT(dSOFT_NAME));
        RegCloseKey(hKey);
    }
}
*/

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qInstallMessageHandler(messageHandler); // Output debug info to qInfo.log

    // Update Block : if the filename of the current instance is new.exe, move me from new.exe to StockWidget.exe (with 1 second sleep)
    if (qApp->applicationName() == "new") {

        QThread::msleep(3000);

        QString current_file = qApp->applicationFilePath();
        QFile file(current_file);
        QString new_file = "StockWidget.exe";

        file_remover(new_file);

        if (file.rename(new_file)) {
            qInfo() << "File renamed successfully.";
        } else {
            qInfo() << "File rename failed.";
        }
        // Start a new instance of the application with close current
        QProcess::startDetached(qApp->applicationDirPath() + "/StockWidget.exe");

        return 0;
    }

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

    // Add a "Autorun" action with CheckBox to the menu
    QAction *autorunAction = trayMenu.addAction("Autorun");
    autorunAction->setCheckable(true);
    autorunAction->setChecked(is_soft_in_autorun());
    // Add the action to toggle autorun to the tray menu
    QObject::connect(autorunAction, &QAction::toggled, [&](bool checked) {
        if (autorunAction->isChecked()) {
            add_soft_to_autorun(app);
        } else {
            remove_soft_from_autorun();
        }
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
    bool use_binance_us = 0;

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
            if (tmp.startsWith("$use_binance_us:")) {

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$use_binance_us:", "");

                use_binance_us = tmp.toInt();

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
            fout << "$use_binance_us:" << "0" << Qt::endl;
            fout << Qt::endl;

            for (QString symbol : symbol_list) {
              fout << symbol << Qt::endl;
            }

            // Close the file.
            file.close();

            // Start a new instance of the application
            QProcess::startDetached(qApp->applicationFilePath());

            // Close the current instance
            return 0;
        } else {
            return -1;
        }
    }

    // Auto-Update Block
    UpdateInfo info = getUpdateInfo();

    QFuture<int> future_update = QtConcurrent::run([&]() {

        QThread::msleep(1000);

        try {
            if (info.update_available == true && version_convert(SOFT_VERSION) < info.latest_version && auto_update == 1) { // Rewrite for async use

                QNetworkAccessManager manager;
                QNetworkRequest request(QUrl(info.update_url));
                QString user_agent = SOFT_NAME + " " + SOFT_VERSION + " " + get_client_id() + " action[download_update]";
                request.setRawHeader("User-Agent", user_agent.toUtf8());
                QNetworkReply *reply = manager.get(request);

                // Wait for the request to complete
                QEventLoop eventLoop;
                QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
                eventLoop.exec();

                if (reply->error() != QNetworkReply::NoError) {
                    qInfo() << "Error downloading file:" << reply->errorString();
                    return -1;
                }

                QByteArray zip_data = reply->readAll();
                QTemporaryFile temp_file;
                if (!temp_file.open()) {
                    qInfo() << "Error creating temp file:" << temp_file.errorString();
                    return -1;
                }
                temp_file.write(zip_data);
                temp_file.close();

                // Calculate SHA256 of the downloaded zip file
                QCryptographicHash hash(QCryptographicHash::Sha256);
                hash.addData(zip_data);
                QString calculated_sha256 = hash.result().toHex();

                // Compare calculated sha256 with the sha256 from the update info
                if (calculated_sha256 != sha256_by_link(info.update_url_sha256)) {
                    qInfo() << "SHA256 doesn't match. Downloaded file might be corrupted.";
                    return -1;
                }

                unzFile zip_file = unzOpen(temp_file.fileName().toUtf8().constData());

                int ret = unzLocateFile(zip_file, "new.exe", 0);
                if (ret != UNZ_OK) {
                    qInfo() << "Error locating new.exe in zip archive";
                    unzClose(zip_file);
                    return -1;
                }

                ret = unzOpenCurrentFile(zip_file);
                if (ret != UNZ_OK) {
                    qInfo() << "Error opening new.exe in zip archive";
                    unzClose(zip_file);
                    return -1;
                }

                QFile outFile("new.exe");
                if (!outFile.open(QIODevice::WriteOnly)) {
                    qInfo() << "Error creating file new.exe";
                    unzCloseCurrentFile(zip_file);
                    unzClose(zip_file);
                    return -1;
                }

                char buffer[4096];
                int readBytes;
                do {
                    readBytes = unzReadCurrentFile(zip_file, buffer, sizeof(buffer));
                    if (readBytes < 0) {
                        qInfo() << "Error reading new.exe from zip archive";
                        unzCloseCurrentFile(zip_file);
                        unzClose(zip_file);
                        outFile.close();
                        return -1;
                    }
                    outFile.write(buffer, readBytes);
                } while (readBytes > 0);

                unzCloseCurrentFile(zip_file);
                unzClose(zip_file);
                outFile.close();

                // Start a new instance of the application with close current
                QProcess::startDetached(qApp->applicationDirPath() + "/new.exe");

                qApp->exit(0);

            }
        } catch (...) {
        }

        return 0;
    });

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

        QString binance_domain = (use_binance_us) ? "api.binance.us" : "api.binance.com";
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
