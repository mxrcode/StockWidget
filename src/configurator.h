#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include "config.h"

#include <QWidget>
#include <QMouseEvent>

namespace Ui {
class Configurator;
}

// Settings window. Edits a copy of the config and emits applied() on "Apply".
class Configurator : public QWidget
{
    Q_OBJECT

public:
    explicit Configurator(QWidget *parent = nullptr);
    ~Configurator();

    void set_config(const Config &config);

signals:
    void applied(const Config &config);

protected:
    // Tracking for MouseEvent : Window Move
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                m_drag_position = QCursor::pos() - frameGeometry().topLeft();
                event->accept();
            }
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                move(QCursor::pos() - m_drag_position);
                event->accept();
            }
        }
    }

private slots:
    void on_closeButton_clicked();
    void on_applyButton_clicked();
    void on_cancelButton_clicked();
    void on_selectColor_button_clicked();

private:
    void show_color(const QColor &color);

    Ui::Configurator *ui;
    QPoint m_drag_position;

    Config m_config;
    QString m_color_label_style;
};

#endif // CONFIGURATOR_H
