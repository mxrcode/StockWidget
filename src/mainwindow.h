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
// or at a free position chosen by dragging it with the mouse
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Keys "exchange:PAIR"
    void set_pairs(const QStringList &keys, const QString &text_style);
    void show_tickers(const TickerMap &tickers);
    void set_color_changes(bool enabled) { m_color_changes = enabled; }
    void set_always_on_top(bool enabled);

    // About : position
    // ⌜¯¯¯¯¯¯¯¯¯¯¯¯⌝
    // | 0,0    0,1 |
    // |            |
    // | D1,0   1,1 |
    // ⌞____________⌟
    // "@x,y" : free position, top left corner of the window
    void place(const QString &position);

signals:
    // The widget was dragged with the mouse, new free position
    void moved(const QString &position);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct Row {
        QString key;
        QLabel *symbol;
        QLabel *price;
        QLabel *price_high_low;
        QLabel *price_change_percent;
        QVLabel *high_low_difference;
        double last_price = 0;
    };

    void clear_rows();
    void show_status(Row &row, const QString &base, const QString &text);
    void flash(QLabel *label, const QString &color);
    void place_on_screen();

    Ui::MainWindow *ui;
    QVector<Row> m_rows;
    QString m_position = "1,0";
    QString m_text_style;
    bool m_color_changes = true;

    // Dragging
    QPoint m_press_offset;
    QPoint m_press_position;
    bool m_dragging = false;
};

#endif // MAINWINDOW_H
