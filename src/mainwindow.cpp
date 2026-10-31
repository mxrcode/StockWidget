#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "config.h"
#include "format.h"
#include "qvlabel.hpp"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QScreen>
#include <QTimer>

static constexpr int kWidth = 420;
static constexpr int kRowHeight = 50;
static constexpr int kVerticalPadding = 10;

// The right part of the window is empty, the widget is moved over the screen edge by this amount
static constexpr int kRightInset = 36;

static constexpr int kFlashDurationMs = 1500;

static const QString kRiseColor = "color: rgba(90, 220, 120, 0.85);";
static const QString kFallColor = "color: rgba(255, 100, 100, 0.85);";

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

    // Screens added, removed or resized (resolution, taskbar)
    auto watch_screen = [this](QScreen *screen) {
        connect(screen, &QScreen::availableGeometryChanged, this, &MainWindow::place_on_screen);
    };
    for (QScreen *screen : QGuiApplication::screens()) {
        watch_screen(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, [this, watch_screen](QScreen *screen) {
        watch_screen(screen);
        place_on_screen();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, &MainWindow::place_on_screen);
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &MainWindow::place_on_screen);
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

void MainWindow::set_pairs(const QStringList &keys, const QString &text_style)
{
    clear_rows();
    m_text_style = text_style;

    auto make_label = [&](QVBoxLayout *layout, int point_size) {
        QLabel *label = new QLabel("Loading...");
        label->setFont(roboto(point_size));
        label->setStyleSheet(text_style);
        layout->addWidget(label);
        return label;
    };

    for (const QString &key : keys) {
        Row row;
        row.key = key;
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

    setFixedSize(kWidth, keys.size() * kRowHeight + kVerticalPadding);
    place_on_screen();
}

void MainWindow::show_status(Row &row, const QString &base, const QString &text)
{
    row.last_price = 0;
    row.symbol->setText(base);
    row.price->setText(text);
    row.price_high_low->setText(text);
    row.price_change_percent->setText(text);
    row.price_change_percent->setStyleSheet(m_text_style);
    row.high_low_difference->setText(text);
}

void MainWindow::flash(QLabel *label, const QString &color)
{
    label->setStyleSheet(color);

    QPointer<QLabel> guard(label);
    const QString text_style = m_text_style;
    QTimer::singleShot(kFlashDurationMs, this, [guard, text_style]() {
        if (guard) guard->setStyleSheet(text_style);
    });
}

void MainWindow::show_tickers(const TickerMap &tickers)
{
    for (Row &row : m_rows) {
        if (!tickers.contains(row.key)) continue;

        const Ticker &ticker = tickers[row.key];
        const QString pair = Config::pair_of(row.key);

        QString base = pair;
        QString quote;
        const int dash = pair.indexOf('-');
        if (dash != -1) {
            base = pair.left(dash);
            quote = pair.mid(dash + 1);
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

        if (m_color_changes) {
            if (ticker.change_percent > 0) {
                row.price_change_percent->setStyleSheet(kRiseColor);
            } else if (ticker.change_percent < 0) {
                row.price_change_percent->setStyleSheet(kFallColor);
            } else {
                row.price_change_percent->setStyleSheet(m_text_style);
            }

            if (row.last_price > 0 && ticker.last != row.last_price) {
                flash(row.price, ticker.last > row.last_price ? kRiseColor : kFallColor);
            }
        } else {
            row.price_change_percent->setStyleSheet(m_text_style);
        }

        row.last_price = ticker.last;
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
    if (Config::is_free_position(m_position)) {
        const QPoint point = Config::free_position(m_position);

        // The monitor may have been disconnected : fall back to the default corner
        QScreen *screen = QGuiApplication::screenAt(point + QPoint(width() / 2, height() / 2));
        if (screen) {
            const QRect area = screen->availableGeometry();
            move(qBound(area.left(), point.x(), area.left() + area.width() - (width() - kRightInset)),
                 qBound(area.top(), point.y(), area.top() + area.height() - height()));
            return;
        }
    }

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

// Only the text is clickable, the transparent part of the window lets the clicks through
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_press_position = event->globalPosition().toPoint();
        m_press_offset = m_press_position - pos();
        m_dragging = false;
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) return;

    const QPoint global = event->globalPosition().toPoint();
    if (!m_dragging && (global - m_press_position).manhattanLength() < QApplication::startDragDistance()) return;

    if (!m_dragging) {
        m_dragging = true;
        setCursor(Qt::SizeAllCursor);
    }
    move(global - m_press_offset);
    event->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_dragging) return;

    m_dragging = false;
    unsetCursor();

    m_position = Config::make_free_position(pos());
    place_on_screen(); // Keep it inside the screen
    m_position = Config::make_free_position(pos());

    emit moved(m_position);
    event->accept();
}
