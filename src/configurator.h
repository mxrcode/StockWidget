#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include "config.h"
#include "exchanges/exchange.h"

#include <QDateTime>
#include <QHash>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QWidget>

class QButtonGroup;
class QListWidgetItem;

namespace Ui {
class Configurator;
}

// Settings window. Edits a copy of the config and emits applied() on "Apply".
// The "Pairs" tab lists the pairs of the chosen exchange with a search.
class Configurator : public QWidget
{
    Q_OBJECT

public:
    explicit Configurator(QWidget *parent = nullptr);
    ~Configurator();

    void set_config(const Config &config);

    // Changes made outside of the window while it is open (dragging the widget, tray menu)
    void set_position(const QString &position);
    void set_always_on_top(bool enabled);

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
    // Pair list of an exchange, kept for a while to reopen the window quickly
    struct Catalog {
        QList<Market> markets;
        QHash<QString, int> index; // pair -> position in markets
        QDateTime loaded;
        bool loading = false;
        bool failed = false;
    };

    QString current_source() const;
    void request_catalog(const QString &source);
    void show_markets();
    void rebuild_quote_buttons(const Catalog &catalog);
    void apply_filter();

    void add_pair(const QString &source, const QString &pair);
    void add_selected_markets();
    void add_from_search();
    void remove_selected_pairs();
    void move_selected_pair(int step);
    void validate_selected_pairs();
    void update_selected_item(QListWidgetItem *item);

    void fill_refresh_intervals(int current);
    void show_color(const QColor &color);

    Ui::Configurator *ui;
    QPoint m_drag_position;

    Config m_config;

    QNetworkAccessManager m_network;
    QHash<QString, Exchange *> m_exchanges;
    QHash<QString, Catalog> m_catalogs;

    QButtonGroup *m_quote_group;
    QString m_quote_filter; // Empty : all quote currencies
};

#endif // CONFIGURATOR_H
