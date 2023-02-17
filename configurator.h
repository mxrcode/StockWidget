#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include "version.h"

#include <QWidget>
#include <QColorDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QFile>
#include <QProcess>

#include "warningui.h"

namespace Ui {
class Configurator;
}

class Configurator : public QWidget
{
    Q_OBJECT

public:
    explicit Configurator(QWidget *parent = nullptr);
    ~Configurator();

protected:
    // Tracking for MouseEvent : Window Move
    void mousePressEvent(QMouseEvent *event) {
        if (event->button() == Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                //m_dragPosition = event->globalPos() - frameGeometry().topLeft(); // globalPos() is deprecated now
                m_drag_position = QCursor::pos() - frameGeometry().topLeft();
                event->accept();
            }
        }
    }
    void mouseMoveEvent(QMouseEvent *event) {
        if (event->buttons() & Qt::LeftButton)
        {
            if (event->pos().y() < 44) {
                // move(event->globalPos() - m_dragPosition); // globalPos() is deprecated now
                move(QCursor::pos() - m_drag_position);
                event->accept();
            }
        }
    }

signals:
    void setWarningUiText(QString);

private slots:
    void on_closeButton_clicked();
    void on_applyButton_clicked();
    void on_cancelButton_clicked();
    void on_selectColor_button_clicked();

    void on_widgetPosition_radioButton__1_clicked();
    void on_widgetPosition_radioButton__2_clicked();
    void on_widgetPosition_radioButton__3_clicked();
    void on_widgetPosition_radioButton__4_clicked();

    void on_auto_update_checkBox_stateChanged(int arg1);

    void on_dataSources_comboBox_currentIndexChanged(int index);

private:
    Ui::Configurator *ui;
    QPoint m_drag_position;

    // Data
    QString m_widget_position;
    QString m_auto_update;
    QString m_text_style;
    QString m_symbols_string;
    QString m_always_on_top = "0";

    QMap<QString, QString> m_data_sources;
    QString m_data_sources_current;

    WarningUi *warning_ui;
};

#endif // CONFIGURATOR_H
