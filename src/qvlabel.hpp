#ifndef QVLABEL_H
#define QVLABEL_H

#include <QWidget>
#include <QPainter>
#include <QTextOption>

// Label with rotated text
class QVLabel : public QWidget
{
public:
    QVLabel(QWidget *parent = nullptr) : QWidget(parent) {}
    void setText(const QString& text = "QVLabel") {
        m_text = text;
        update();
    }
    void rotate(const int& angle = 90) {
        m_angle = angle;
        update();
    }
    void setXY(const int& x = 0, const int& y = 0) {
        m_x = x;
        m_y = y;
        update();
    }
    void setXYWH(const int& x = 0, const int& y = 0, const int& w = 0, const int& h = 0) {
        m_x = x;
        m_y = y;
        m_w = w;
        m_h = h;
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        QTransform transform;
        transform.rotate(m_angle);
        painter.setTransform(transform);

        QTextOption option;
        option.setAlignment(Qt::AlignCenter);
        painter.drawText(QRectF(m_x, m_y, m_w, m_h), m_text, option);
    }
private:
    QString m_text;
    int m_angle = 0;
    int m_x = 0;
    int m_y = 0;
    int m_w = 0;
    int m_h = 0;
};

#endif // QVLABEL_H
