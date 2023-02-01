#ifndef ABOUTQT_H
#define ABOUTQT_H

#include "version.h"

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>

namespace Ui {
class AboutQt;
}

class AboutQt : public QWidget
{
    Q_OBJECT

public:
    explicit AboutQt(QWidget *parent = nullptr);
    ~AboutQt();

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

private slots:
    void on_closeButton_clicked();

private:
    Ui::AboutQt *ui;
    QPoint m_dragPosition;
};

#endif // ABOUTQT_H
