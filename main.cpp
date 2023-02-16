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
    QString decimal_part = parts[1];
    decimal_part.chop(decimal_part.size() - decimal_places);
    number = parts[0] + "." + decimal_part;

    // Insert a space every 3 digits
    int dot_index = number.indexOf(".");
    if (dot_index == -1) return number; // number doesn't contain a dot

    for (int i = dot_index - 3; i > 0; i -= 3) {
        number.insert(i, ' ');
    }

    // Trim trailing zeros after dot
    while (number.endsWith("0") && number.at(dot_index + 1) != '\0') {
        if (number.at(number.size()-3) == '.') break;
        number.chop(1);
    }
    if (number.endsWith(".")) number.chop(1);

    return number;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) // Part for outputting debug info to qInfo.log
{
    QFile log_file("qInfo.log");
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&log_file);
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
        IPersistFile *persist_file;
        shell_link->SetPath(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(app.applicationFilePath()).utf16()));
        shell_link->SetWorkingDirectory(reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(app.applicationDirPath()).utf16()));
        result = shell_link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&persist_file));
        if (result == S_OK) {
            persist_file->Save(reinterpret_cast<const wchar_t *>(lnk_path.utf16()), TRUE);
            persist_file->Release();
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

void replace_or_add_str_to_file (const QString& filename, const QString& search_str, const QString& new_str) {

    QFile file(filename);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qInfo() << "Failed to open file " << filename;
        return;
    }

    bool string_found = false;

    QTextStream in(&file);
    QString line = in.readLine();
    QString file_contents;

    while (!line.isNull()) {
        if (line.startsWith(search_str)) {
            string_found = true;
            file_contents.append(new_str + "\n");
        } else {
            file_contents.append(line + "\n");
        }

        line = in.readLine();
    }

    if (!string_found) {
        file_contents.append(new_str + "\n");
    }

    file.resize(0);
    in << file_contents;
}

QMap<QString, QMap<QString, QString>> get_exchange_data(QString current_sources, QVector<QString> symbol_list)
{
    QMap<QString, QMap<QString, QString>> output;

    if (current_sources == "binance_com") { // Binance Global

        QJsonArray symbols_json;
        for (const auto &symbol : symbol_list) {
            QString raw = symbol;
            raw.replace("-", "");
            symbols_json.append(raw);
        }
        QJsonDocument json_doc(symbols_json);
        QString json_string = json_doc.toJson(QJsonDocument::Compact);

        // Create a network manager.
        QNetworkAccessManager manager;

        QNetworkRequest request(QUrl("https://api.binance.com/api/v3/ticker/24hr?symbols=" + json_string));
        auto *reply = manager.get(request);

        // Wait for the request to complete
        QEventLoop event_loop;
        QObject::connect(reply, &QNetworkReply::finished, &event_loop, &QEventLoop::quit);
        event_loop.exec();

        // Get the response data.
        QByteArray data = reply->readAll();

        // Parse the JSON data
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray json_array = doc.array();

        for (int i = 0; i < symbol_list.size(); ++i) {

            QJsonValue values = json_array[i];
            QJsonObject obj = values.toObject();

            QString current_symbol = obj["symbol"].toString();

            QMap<QString, QString> inner_map;
            inner_map.insert("symbol", current_symbol);
            inner_map.insert("price", digit_format(obj["lastPrice"].toString()));
            inner_map.insert("price_24h", digit_format(obj["highPrice"].toString()));
            inner_map.insert("price_24l", digit_format(obj["lowPrice"].toString()));
            inner_map.insert("price_percent_change", digit_format(obj["priceChangePercent"].toString()));

            // Set High/Low 24h price difference
            double d_H24 = obj["highPrice"].toString().toDouble();
            double d_L24 = obj["lowPrice"].toString().toDouble();
            inner_map.insert("price_difference", digit_format( QString::number((d_H24-d_L24)/(d_L24/100))));

            for (QString symbol : symbol_list) {
                QString raw = symbol;
                raw.replace("-", "");

                if (raw == current_symbol) {

                    QStringList parts = symbol.split("-");
                    inner_map.insert("coin", parts[0]);

                    output.insert(symbol, inner_map);break;
                }
            }

        }

    }

    return output;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qInstallMessageHandler(messageHandler); // Output debug info to qInfo.log

    // Create the main window
    MainWindow mainWindow;
    Ui::MainWindow *ui = mainWindow.ui;
    mainWindow.show();

    // Update Block : if the filename of the current instance is new.exe, move me from new.exe to StockWidget.exe (with 1 second sleep)
    if (qApp->applicationName() == "new") {

        QThread::msleep(3000);

        // Somewhere here, need to add code that removes the old version CONFIG_NAME.

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

    QString text_style = "color:rgba(255, 255, 255, 0.65);";
    QString widget_position = "1,0";

    bool auto_update = 1;
    bool always_on_top = 0;

    QString data_sources_current = "binance_com";

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
            if (tmp.startsWith("$always_on_top:")) { // ALWAYS ON TOP

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$always_on_top:", "");

                always_on_top = tmp.toInt();

                continue;
            }
            if (tmp.startsWith("$data_sources:")) { // DATA SOURCES

                // Delete comments after "//"
                int index = tmp.indexOf("//");
                if (index != -1) tmp = tmp.left(index);

                tmp.replace("$data_sources:", "");

                data_sources_current = tmp;

                continue;
            }
            if (tmp.startsWith("$")) continue; // Skip all other parameters

            symbol_list.append(tmp);
        }
        file.close();
    } else {
        symbol_list = {
            "BTC-USDT",
            "ETH-USDT",
            "LTC-USDT",
            "BNB-USDT",
            "CAKE-USDT",
        };

        // Open the file for writing.
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            // The file was opened successfully.
            QTextStream fout(&file);

            // Write default config to the file.
            fout << "// " << SOFT_NAME << " " << SOFT_VERSION << " : One line - One trading pair." << Qt::endl;
            fout << Qt::endl;
            fout << "$text_style:color:rgba(255, 255, 255, 0.65);" << Qt::endl;
            fout << "$position:" << widget_position << Qt::endl;
            fout << "$auto_update:" << "1" << Qt::endl;
            fout << "$always_on_top:" << "0" << Qt::endl;
            fout << "$data_sources:" << data_sources_current << Qt::endl;
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

    // Tray Block
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
    // Add the action to toggle "Autorun" to the tray menu
    QObject::connect(autorunAction, &QAction::toggled, [&](bool checked) {
        if (checked) {
            add_soft_to_autorun(app);
        } else {
            remove_soft_from_autorun();
        }
    });

    // Add a "Always on top" action with CheckBox to the menu
    QAction *always_on_topAction = trayMenu.addAction("Always on top");
    always_on_topAction->setCheckable(true);
    always_on_topAction->setChecked(always_on_top);
    if (always_on_top) { // Set current state for Qt::WindowStaysOnTopHint
        mainWindow.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        mainWindow.icon_taskbar_hider();
        mainWindow.show();
    }
    // Add the action to toggle "Always on top" to the tray menu
    QObject::connect(always_on_topAction, &QAction::toggled, [&](bool checked) {
        if (checked) {
            replace_or_add_str_to_file(CONFIG_NAME, "$always_on_top:0", "$always_on_top:1");
            mainWindow.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        } else {
            replace_or_add_str_to_file(CONFIG_NAME, "$always_on_top:1", "$always_on_top:0");
            mainWindow.setWindowFlag(Qt::WindowStaysOnTopHint, false);
        }
        mainWindow.icon_taskbar_hider();
        mainWindow.show();
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
    labels_map.insert("price_HL", {});
    labels_map.insert("price_percent_change", {});

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
        } else if (label_list == "price_HL") {
            for (QLabel *label : labels_map[label_list]) {
                ui->verticalLayout_priceHighLow->addWidget(label);
            }
        } else if (label_list == "price_percent_change") {
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

        if (i_connect == 0) {
            timer->setInterval(60*1000); // update every X ms
            i_connect = 1;
        }

        QMap<QString, QMap<QString, QString>> exchange_data = get_exchange_data(data_sources_current, symbol_list);

        for (int i = 0; i < symbol_list.size(); ++i) {

            QFont fontRoboto_18("Roboto", 18, QFont::Normal);
            fontRoboto_18.setStyleStrategy(QFont::PreferAntialias);
            labels_map["symbol"].at(i)->setFont(fontRoboto_18);
            labels_map["symbol"].at(i)->setText(exchange_data[symbol_list[i]]["coin"]);

            labels_map["price"].at(i)->setText("$" + exchange_data[symbol_list[i]]["price"]);

            // Set coin High & Low price
            QFont fontRoboto_8("Roboto", 8, QFont::Normal);
            fontRoboto_8.setStyleStrategy(QFont::PreferAntialias);
            labels_map["price_HL"].at(i)->setFont(fontRoboto_8);
            labels_map["price_HL"].at(i)->setText("$" + exchange_data[symbol_list[i]]["price_24h"] + "<br>" + "$" + exchange_data[symbol_list[i]]["price_24l"]);

            labels_map["price_percent_change"].at(i)->setText("%" + exchange_data[symbol_list[i]]["price_percent_change"]);

            // Set coin High/Low price difference
            hl_difference.at(i)->setText("%" + exchange_data[symbol_list[i]]["price_difference"]);
            hl_difference.at(i)->repaint();

        }

    });

    return app.exec();
}
