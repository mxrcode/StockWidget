#ifndef WARNINGUI_H
#define WARNINGUI_H

#include "version.h"

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>

namespace Ui {
class WarningUi;
}

class WarningUi : public QWidget
{
    Q_OBJECT

public:
    explicit WarningUi(QWidget *parent = nullptr);
    ~WarningUi();

protected:
    // Tracking for MouseEvent : Window Move
    void mousePressEvent(QMouseEvent *event) {
        if (event->button() == Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                //m_dragPosition = event->globalPos() - frameGeometry().topLeft(); // globalPos() is deprecated now
                m_dragPosition = QCursor::pos() - frameGeometry().topLeft();
                event->accept();
            }
        }
    }
    void mouseMoveEvent(QMouseEvent *event) {
        if (event->buttons() & Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                // move(event->globalPos() - m_dragPosition); // globalPos() is deprecated now
                move(QCursor::pos() - m_dragPosition);
                event->accept();
            }
        }
    }

public slots:
    void setTextBrowser(QString text);

private slots:
    void on_closeButton_clicked();
    void on_OkButton_clicked();

private:
    Ui::WarningUi *ui;
    QPoint m_dragPosition;
};

#endif // WARNINGUI_H
