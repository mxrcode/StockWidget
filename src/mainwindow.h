#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ticker.h"

#include <QMainWindow>
#include <QStringList>
#include <QVector>

class QLabel;
class QVLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// The widget itself : one row per trading pair, placed in a corner of the primary screen
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void set_pairs(const QStringList &pairs, const QString &text_style);
    void show_tickers(const TickerMap &tickers);
    void set_always_on_top(bool enabled);

    // About : position
    // ⌜¯¯¯¯¯¯¯¯¯¯¯¯⌝
    // | 0,0    0,1 |
    // |            |
    // | D1,0   1,1 |
    // ⌞____________⌟
    void place(const QString &position);

private:
    struct Row {
        QString pair;
        QLabel *symbol;
        QLabel *price;
        QLabel *price_high_low;
        QLabel *price_change_percent;
        QVLabel *high_low_difference;
    };

    void clear_rows();
    void show_status(const Row &row, const QString &base, const QString &text);
    void place_on_screen();

    Ui::MainWindow *ui;
    QVector<Row> m_rows;
    QString m_position = "1,0";
};

#endif // MAINWINDOW_H
