#include "configurator.h"
#include "ui_configurator.h"

#include "format.h"
#include "version.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QHeaderView>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>

#include <algorithm>
#include <numeric>

// Text color is always drawn with this opacity
static const QString kTextOpacity = "0.65";

// Pair lists are reloaded after this time
static constexpr int kCatalogLifetimeSecs = 10 * 60;

// Number of quote currency filter buttons besides "All"
static constexpr int kQuoteButtons = 6;

static const QColor kMissingPairColor("#FF8A80");
static const QColor kRiseColor("#7BD88F");
static const QColor kFallColor("#FF8A80");

enum MarketRole { PairRole = Qt::UserRole, QuoteRole };

// "color: rgba(255, 255, 255, 0.65);" -> QColor(255, 255, 255)
static QColor color_from_style(const QString &style)
{
    static const QRegularExpression regex("rgba?\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)");

    const QRegularExpressionMatch match = regex.match(style);
    if (!match.hasMatch()) return QColor(Qt::white);

    return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
}

static QString quote_of(const QString &pair)
{
    return pair.section('-', 1);
}

// Rough 24h volume in USD, only used to show the popular pairs first
static QVector<double> usd_volumes(const QList<Market> &markets, const QHash<QString, int> &index)
{
    QHash<QString, double> usd_rates;
    auto usd_rate = [&](const QString &quote) {
        if (usd_rates.contains(quote)) return usd_rates[quote];

        double rate = 0;
        if (currency_prefix(quote) == "$") {
            rate = 1;
        } else {
            for (const char *usd : {"USDT", "USD", "USDC"}) {
                const int i = index.value(quote + "-" + QLatin1String(usd), -1);
                if (i != -1 && markets[i].last > 0) {
                    rate = markets[i].last;
                    break;
                }
            }
        }
        usd_rates.insert(quote, rate);
        return rate;
    };

    QVector<double> volumes;
    volumes.reserve(markets.size());
    for (const Market &market : markets) {
        volumes.append(market.quote_volume * usd_rate(quote_of(market.pair)));
    }
    return volumes;
}

Configurator::Configurator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Configurator),
    m_quote_group(new QButtonGroup(this))
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    ui->windowName->setText(this->windowTitle() + " — " + SOFT_NAME + " " + SOFT_VERSION);

    for (const auto &source : Exchange::available()) {
        ui->exchange_comboBox->addItem(source.second, source.first);
    }

    QHeaderView *header = ui->markets_treeWidget->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    connect(ui->exchange_comboBox, &QComboBox::currentIndexChanged, this, [this]() {
        request_catalog(current_source());
        show_markets();
    });
    connect(ui->search_lineEdit, &QLineEdit::textChanged, this, &Configurator::apply_filter);
    connect(ui->search_lineEdit, &QLineEdit::returnPressed, this, &Configurator::add_from_search);
    connect(ui->markets_treeWidget, &QTreeWidget::itemActivated, this, &Configurator::add_selected_markets);
    connect(ui->addButton, &QPushButton::clicked, this, &Configurator::add_selected_markets);
    connect(ui->removeButton, &QPushButton::clicked, this, &Configurator::remove_selected_pairs);
    connect(ui->upButton, &QPushButton::clicked, this, [this]() { move_selected_pair(-1); });
    connect(ui->downButton, &QPushButton::clicked, this, [this]() { move_selected_pair(1); });
    new QShortcut(QKeySequence::Delete, ui->selected_listWidget, [this]() { remove_selected_pairs(); }, Qt::WidgetShortcut);

    connect(m_quote_group, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *button) {
        m_quote_filter = button->property("quote_value").toString();
        apply_filter();
    });
}

Configurator::~Configurator()
{
    delete ui;
}

void Configurator::set_config(const Config &config)
{
    m_config = config;

    // Pairs
    ui->selected_listWidget->clear();
    for (const QString &key : config.pairs) {
        add_pair(Config::source_of(key), Config::pair_of(key));
    }
    ui->selected_listWidget->clearSelection();
    ui->selected_listWidget->setCurrentRow(-1);

    const int source_index = ui->exchange_comboBox->findData(config.data_source);
    ui->exchange_comboBox->setCurrentIndex(source_index == -1 ? 0 : source_index);

    request_catalog(current_source());
    for (const QString &key : config.pairs) {
        request_catalog(Config::source_of(key));
    }
    show_markets();

    // Appearance
    set_position(config.position);
    show_color(color_from_style(config.text_style));
    ui->color_changes_checkBox->setChecked(config.color_changes);

    // Other
    fill_refresh_intervals(config.refresh_interval);
    ui->always_on_top_checkBox->setChecked(config.always_on_top);
    ui->error_notifications_checkBox->setChecked(config.error_notifications);
}

void Configurator::set_position(const QString &position)
{
    m_config.position = position;

    const bool free = Config::is_free_position(position);
    ui->custom_radioButton->setEnabled(free);

    if (free) {
        ui->custom_radioButton->setChecked(true);
    } else if (position == "0,0") {
        ui->topLeft_radioButton->setChecked(true);
    } else if (position == "0,1") {
        ui->topRight_radioButton->setChecked(true);
    } else if (position == "1,1") {
        ui->bottomRight_radioButton->setChecked(true);
    } else { // 1,0 : Default
        ui->bottomLeft_radioButton->setChecked(true);
    }
}

QString Configurator::current_source() const
{
    return ui->exchange_comboBox->currentData().toString();
}

void Configurator::request_catalog(const QString &source)
{
    if (!Exchange::is_available(source)) return;

    Catalog &catalog = m_catalogs[source];
    if (catalog.loading) return;
    if (!catalog.failed && catalog.loaded.isValid() && catalog.loaded.secsTo(QDateTime::currentDateTime()) < kCatalogLifetimeSecs) return;

    catalog.loading = true;
    catalog.failed = false;

    Exchange *&exchange = m_exchanges[source];
    if (!exchange) exchange = Exchange::create(source, &m_network, this);

    exchange->load_markets([this, source](const QList<Market> &markets, bool ok) {
        Catalog &catalog = m_catalogs[source];
        catalog.loading = false;

        if (ok) {
            catalog.markets = markets;
            catalog.index.clear();
            for (int i = 0; i < markets.size(); ++i) {
                catalog.index.insert(markets[i].pair, i);
            }
            catalog.loaded = QDateTime::currentDateTime();
        }
        // An outdated list is still better than none
        catalog.failed = !ok && catalog.markets.isEmpty();

        if (source == current_source()) show_markets();
        validate_selected_pairs();
    });
}

void Configurator::show_markets()
{
    QTreeWidget *tree = ui->markets_treeWidget;
    tree->clear();

    const QString source = current_source();
    const Catalog catalog = m_catalogs.value(source);
    const QString name = Exchange::display_name(source);

    rebuild_quote_buttons(catalog);

    if (catalog.markets.isEmpty()) {
        if (catalog.failed) {
            ui->markets_status_label->setText("Could not load the pairs of " + name + ". You can still type a pair (e.g. BTC-USDT) and press Enter.");
        } else {
            ui->markets_status_label->setText("Loading the pairs of " + name + "...");
        }
        return;
    }

    // Popular pairs first
    const QVector<double> volumes = usd_volumes(catalog.markets, catalog.index);
    QVector<int> order(catalog.markets.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (volumes[a] != volumes[b]) return volumes[a] > volumes[b];
        return catalog.markets[a].pair < catalog.markets[b].pair;
    });

    QList<QTreeWidgetItem *> items;
    items.reserve(order.size());
    for (int i : order) {
        const Market &market = catalog.markets[i];
        const QString quote = quote_of(market.pair);

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, market.pair);
        item->setData(0, PairRole, market.pair);
        item->setData(0, QuoteRole, quote);

        if (market.last > 0) {
            item->setText(1, currency_prefix(quote) + format_number(market.last));
            item->setText(2, (market.change_percent > 0 ? "+" : "") + format_number(market.change_percent, 2) + "%");
            if (market.change_percent > 0) item->setForeground(2, kRiseColor);
            if (market.change_percent < 0) item->setForeground(2, kFallColor);
        }
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);

        items.append(item);
    }
    tree->addTopLevelItems(items);

    apply_filter();
}

void Configurator::rebuild_quote_buttons(const Catalog &catalog)
{
    while (QLayoutItem *item = ui->quoteLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (catalog.markets.isEmpty()) return;

    // Well-known quote currencies first, then the most common ones of the exchange
    QHash<QString, int> counts;
    for (const Market &market : catalog.markets) {
        counts[quote_of(market.pair)]++;
    }

    QStringList quotes;
    for (const char *quote : {"USDT", "USD", "USDC", "EUR", "BTC", "ETH"}) {
        if (counts.contains(quote)) quotes.append(quote);
    }

    QStringList others = counts.keys();
    std::sort(others.begin(), others.end(), [&](const QString &a, const QString &b) {
        return counts[a] != counts[b] ? counts[a] > counts[b] : a < b;
    });
    for (const QString &quote : others) {
        if (!quotes.contains(quote)) quotes.append(quote);
    }
    quotes = quotes.mid(0, kQuoteButtons);

    if (!quotes.contains(m_quote_filter)) m_quote_filter.clear();

    quotes.prepend(QString()); // "All"
    for (const QString &quote : quotes) {
        QPushButton *button = new QPushButton(quote.isEmpty() ? "All" : quote);
        button->setCheckable(true);
        button->setAutoDefault(false);
        button->setProperty("quote", true); // Style
        button->setProperty("quote_value", quote);
        button->setChecked(quote == m_quote_filter);
        m_quote_group->addButton(button);
        ui->quoteLayout->addWidget(button);
    }
    ui->quoteLayout->addStretch();
}

void Configurator::apply_filter()
{
    QTreeWidget *tree = ui->markets_treeWidget;
    if (tree->topLevelItemCount() == 0) return;

    const QString text = Config::normalize_pair(ui->search_lineEdit->text());
    const QString compact_text = QString(text).remove('-');

    int visible = 0;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        const QString pair = item->data(0, PairRole).toString();

        bool show = m_quote_filter.isEmpty() || item->data(0, QuoteRole).toString() == m_quote_filter;
        if (show && !text.isEmpty()) {
            show = pair.contains(text) || QString(pair).remove('-').contains(compact_text);
        }

        item->setHidden(!show);
        if (show) visible++;
    }

    ui->markets_status_label->setText(QString("%1 of %2 pairs on %3. Double-click or → to add.")
                                      .arg(visible).arg(tree->topLevelItemCount()).arg(Exchange::display_name(current_source())));
}

void Configurator::add_pair(const QString &source, const QString &pair)
{
    const QString key = Config::pair_key(source, pair);
    QListWidget *list = ui->selected_listWidget;

    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->data(Qt::UserRole).toString() == key) {
            list->setCurrentRow(i);
            return;
        }
    }

    QListWidgetItem *item = new QListWidgetItem;
    item->setData(Qt::UserRole, key);
    update_selected_item(item);
    list->addItem(item);
    list->setCurrentItem(item);
}

void Configurator::add_selected_markets()
{
    for (QTreeWidgetItem *item : ui->markets_treeWidget->selectedItems()) {
        add_pair(current_source(), item->data(0, PairRole).toString());
    }
}

void Configurator::add_from_search()
{
    const QString text = Config::normalize_pair(ui->search_lineEdit->text());
    if (text.isEmpty()) return;

    const Catalog &catalog = m_catalogs[current_source()];

    QString pair;
    if (catalog.index.contains(text)) {
        pair = text;
    } else {
        // The only pair that matches the search
        QTreeWidget *tree = ui->markets_treeWidget;
        QTreeWidgetItem *match = nullptr;
        int matches = 0;
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            if (!tree->topLevelItem(i)->isHidden()) {
                match = tree->topLevelItem(i);
                matches++;
            }
        }

        if (matches == 1) {
            pair = match->data(0, PairRole).toString();
        } else if (text.count('-') == 1 && !text.startsWith('-') && !text.endsWith('-')) {
            pair = text; // Typed manually, checked against the list when it is loaded
        }
    }

    if (pair.isEmpty()) return;

    add_pair(current_source(), pair);
    ui->search_lineEdit->clear();
}

void Configurator::remove_selected_pairs()
{
    qDeleteAll(ui->selected_listWidget->selectedItems());
}

void Configurator::move_selected_pair(int step)
{
    QListWidget *list = ui->selected_listWidget;
    const int row = list->currentRow();
    const int new_row = row + step;
    if (row < 0 || new_row < 0 || new_row >= list->count()) return;

    QListWidgetItem *item = list->takeItem(row);
    list->insertItem(new_row, item);
    list->setCurrentRow(new_row);
}

void Configurator::validate_selected_pairs()
{
    QListWidget *list = ui->selected_listWidget;
    for (int i = 0; i < list->count(); ++i) {
        update_selected_item(list->item(i));
    }
}

void Configurator::update_selected_item(QListWidgetItem *item)
{
    const QString key = item->data(Qt::UserRole).toString();
    const QString source = Config::source_of(key);
    const QString pair = Config::pair_of(key);
    const QString name = Exchange::display_name(source);

    item->setText(pair + "  ·  " + name);

    const Catalog catalog = m_catalogs.value(source);
    const bool missing = !Exchange::is_available(source) || (!catalog.markets.isEmpty() && !catalog.index.contains(pair));

    if (missing) {
        item->setForeground(kMissingPairColor);
        item->setToolTip("Not found on " + name);
    } else {
        item->setForeground(QBrush());
        item->setToolTip(QString());
    }
}

void Configurator::fill_refresh_intervals(int current)
{
    QList<int> intervals = {10, 15, 30, 60, 120, 300, 600};
    if (!intervals.contains(current)) {
        intervals.append(current);
        std::sort(intervals.begin(), intervals.end());
    }

    ui->refresh_comboBox->clear();
    for (int seconds : intervals) {
        QString text;
        if (seconds % 60 != 0) {
            text = QString("%1 seconds").arg(seconds);
        } else if (seconds == 60) {
            text = "1 minute";
        } else {
            text = QString("%1 minutes").arg(seconds / 60);
        }
        ui->refresh_comboBox->addItem(text, seconds);
    }
    ui->refresh_comboBox->setCurrentIndex(ui->refresh_comboBox->findData(current));
}

void Configurator::show_color(const QColor &color)
{
    ui->currentColor_label->setStyleSheet("border: 1px solid #7A836E; background-color: " + color.name() + ";");
}

void Configurator::on_closeButton_clicked()
{
    this->close();
}

void Configurator::on_cancelButton_clicked()
{
    this->close();
}

void Configurator::on_applyButton_clicked()
{
    Config config = m_config;

    config.pairs.clear();
    for (int i = 0; i < ui->selected_listWidget->count(); ++i) {
        config.pairs.append(ui->selected_listWidget->item(i)->data(Qt::UserRole).toString());
    }

    // Pairs of this exchange are written without the "exchange:" prefix
    config.data_source = current_source();

    if (ui->topLeft_radioButton->isChecked()) {
        config.position = "0,0";
    } else if (ui->topRight_radioButton->isChecked()) {
        config.position = "0,1";
    } else if (ui->bottomRight_radioButton->isChecked()) {
        config.position = "1,1";
    } else if (ui->bottomLeft_radioButton->isChecked()) {
        config.position = "1,0";
    } // Custom : keep the dragged position

    config.color_changes = ui->color_changes_checkBox->isChecked();
    config.refresh_interval = ui->refresh_comboBox->currentData().toInt();
    config.always_on_top = ui->always_on_top_checkBox->isChecked();
    config.error_notifications = ui->error_notifications_checkBox->isChecked();

    this->close();
    emit applied(config);
}

void Configurator::on_selectColor_button_clicked()
{
    const QColor color = QColorDialog::getColor(color_from_style(m_config.text_style), nullptr, "Select Color");

    // Check if the user selected a color
    if (color.isValid()) {
        m_config.text_style = "color: rgba(" + QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue()) + "," + kTextOpacity + ");";
        show_color(color);
    }
}
