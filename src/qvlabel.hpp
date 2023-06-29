#ifndef QVLABEL_H
#define QVLABEL_H

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSize>
#include <QPainter>
#include <QSize>
#include <QTextOption>
#include <QPointF>

class QVLabel : public QWidget
{
public:
    QVLabel(QWidget *parent = nullptr) : QWidget(parent) {}
    void setText(const QString& text = "QVLabel") {
        m_text = text;
    }
    void rotate(const int& angle = 90) {
        m_angle = angle;
    }
    void setXY(const int& x = 0, const int& y = 0) {
        m_x = x;
        m_y = y;
    }
    void setXYWH(const int& x = 0, const int& y = 0, const int& w = 0, const int& h = 0) {
        m_x = x;
        m_y = y;
        m_w = w;
        m_h = h;
    }
protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        QTransform transform;
        transform.rotate(m_angle);
        painter.setTransform(transform);

        QRectF rect;
        rect.setX(m_x);
        rect.setY(m_y);
        rect.setWidth(m_w);
        rect.setHeight(m_h);

        QTextOption option;
        option.setAlignment(Qt::AlignCenter);
        painter.drawText(rect, m_text, option);
    }
private:
    QString m_text;
    int m_angle;
    int m_x;
    int m_y;
    int m_w;
    int m_h;
};

#endif // QVLABEL_H
