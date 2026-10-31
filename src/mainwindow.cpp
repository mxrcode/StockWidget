#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "format.h"
#include "qvlabel.hpp"

#include <QGuiApplication>
#include <QLabel>
#include <QScreen>

static constexpr int kWidth = 420;
static constexpr int kRowHeight = 50;
static constexpr int kVerticalPadding = 10;

// The right part of the window is empty, the widget is moved over the screen edge by this amount
static constexpr int kRightInset = 36;

static QFont roboto(int point_size)
{
    QFont font("Roboto", point_size, QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Qt::Tool : no taskbar button
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    connect(qApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *screen) {
        connect(screen, &QScreen::availableGeometryChanged, this, &MainWindow::place_on_screen);
        place_on_screen();
    });
    connect(QGuiApplication::primaryScreen(), &QScreen::availableGeometryChanged, this, &MainWindow::place_on_screen);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::clear_rows()
{
    for (const Row &row : m_rows) {
        delete row.symbol;
        delete row.price;
        delete row.price_high_low;
        delete row.price_change_percent;
        delete row.high_low_difference;
    }
    m_rows.clear();
}

void MainWindow::set_pairs(const QStringList &pairs, const QString &text_style)
{
    clear_rows();

    auto make_label = [&](QVBoxLayout *layout, int point_size) {
        QLabel *label = new QLabel("Loading...");
        label->setFont(roboto(point_size));
        label->setStyleSheet(text_style);
        layout->addWidget(label);
        return label;
    };

    for (const QString &pair : pairs) {
        Row row;
        row.pair = pair;
        row.symbol = make_label(ui->verticalLayout_symbol, 18);
        row.price = make_label(ui->verticalLayout_price, 14);
        row.price_high_low = make_label(ui->verticalLayout_priceHighLow, 8);
        row.price_change_percent = make_label(ui->verticalLayout_priceChangePercent, 14);

        row.high_low_difference = new QVLabel;
        row.high_low_difference->setText("NTWRK");
        row.high_low_difference->resize(50, 50);
        row.high_low_difference->rotate(270);
        row.high_low_difference->setXYWH(-48, -20, 50, 50);
        row.high_low_difference->setFont(roboto(10));
        row.high_low_difference->setStyleSheet(text_style);
        ui->verticalLayout_difference->addWidget(row.high_low_difference);

        m_rows.append(row);
    }

    setFixedSize(kWidth, pairs.size() * kRowHeight + kVerticalPadding);
    place_on_screen();
}

void MainWindow::show_status(const Row &row, const QString &base, const QString &text)
{
    row.symbol->setText(base);
    row.price->setText(text);
    row.price_high_low->setText(text);
    row.price_change_percent->setText(text);
    row.high_low_difference->setText(text);
}

void MainWindow::show_tickers(const TickerMap &tickers)
{
    for (const Row &row : m_rows) {
        if (!tickers.contains(row.pair)) continue;

        const Ticker &ticker = tickers[row.pair];

        QString base = row.pair;
        QString quote;
        const int dash = row.pair.indexOf('-');
        if (dash != -1) {
            base = row.pair.left(dash);
            quote = row.pair.mid(dash + 1);
        }

        if (ticker.status == Ticker::Status::NetworkError) {
            show_status(row, base, "net");
            continue;
        }
        if (ticker.status == Ticker::Status::UnknownPair) {
            show_status(row, base, "ERROR");
            continue;
        }

        const QString prefix = currency_prefix(quote);
        const double difference = ticker.low > 0 ? (ticker.high - ticker.low) / ticker.low * 100 : 0;

        row.symbol->setText(base);
        row.price->setText(prefix + format_number(ticker.last));
        row.price_high_low->setText(prefix + format_number(ticker.high) + "<br>" + prefix + format_number(ticker.low));
        row.price_change_percent->setText("%" + format_number(ticker.change_percent, 2));
        row.high_low_difference->setText("%" + format_number(difference, 2));
    }
}

void MainWindow::set_always_on_top(bool enabled)
{
    // Changing the flags hides the window
    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    show();
    place_on_screen();
}

void MainWindow::place(const QString &position)
{
    m_position = position;
    place_on_screen();
}

void MainWindow::place_on_screen()
{
    const QRect area = QGuiApplication::primaryScreen()->availableGeometry();

    const int left = area.left();
    const int right = area.left() + area.width() - (width() - kRightInset);
    const int top = area.top();
    const int bottom = area.top() + area.height() - height();

    if (m_position == "0,0") {
        move(left, top);
    } else if (m_position == "0,1") {
        move(right, top);
    } else if (m_position == "1,1") {
        move(right, bottom);
    } else { // 1,0 : Default
        move(left, bottom);
    }
}
