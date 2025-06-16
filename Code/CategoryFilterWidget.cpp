#include "CategoryFilterWidget.h"
#include "MovieCard.h"
#include "Theme.h"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QUrlQuery>
#include <QVBoxLayout>

CategoryFilterWidget::CategoryFilterWidget(QNetworkAccessManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
{
    setupUI();
    loadFilters();
}

CategoryFilterWidget::~CategoryFilterWidget() {}
void CategoryFilterWidget::setupUI()
{
    setStyleSheet(QString("background-color: transparent;"));
    setMinimumSize(800, 600);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        QString(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollBar:vertical { border: none; background: %1; width: 10px; margin: 0px; "
            "border-radius: 5px; }"
            "QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 5px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }")
            .arg(AppTheme::DARK_GREY, AppTheme::PRIMARY_RED));

    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet(QString("background-color: %1;").arg(AppTheme::MAIN_WINDOW_BG));
    m_scrollContent->setMinimumSize(800, 600);
    m_scrollArea->setWidget(m_scrollContent);
    pageLayout->addWidget(m_scrollArea);

    m_contentVBoxLayout = new QVBoxLayout();
    m_contentVBoxLayout->setContentsMargins(25, 20, 25, 20);
    m_contentVBoxLayout->setSpacing(25);
    m_scrollContent->setLayout(m_contentVBoxLayout);

    QLabel *titleLabel = new QLabel("Explore & Filter", m_scrollContent);
    titleLabel->setStyleSheet(
        QString("color: %1; font-size: 28px; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK));
    titleLabel->setAlignment(Qt::AlignHCenter);
    m_contentVBoxLayout->addWidget(titleLabel);

    m_filterToggleButton = new QPushButton("Show Filters", m_scrollContent);
    m_filterToggleButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border: none; border-radius: 8px; "
                "padding: 12px 24px; font-weight: bold; }"
                "QPushButton:hover { background-color: %3; }")
            .arg(AppTheme::PRIMARY_RED, AppTheme::TEXT_ON_DARK, AppTheme::BUTTON_HOVER_DARK_BG));

    connect(m_filterToggleButton,
            &QPushButton::clicked,
            this,
            &CategoryFilterWidget::toggleFilterVisibility);
    m_contentVBoxLayout->addWidget(m_filterToggleButton);

    m_filterContainerWidget = new QWidget(m_scrollContent);
    m_filterContainerWidget->setVisible(false);

    m_filterFormLayout = new QFormLayout(m_filterContainerWidget);
    m_filterFormLayout->setContentsMargins(0, 10, 0, 10);
    m_filterFormLayout->setHorizontalSpacing(20);
    m_filterFormLayout->setVerticalSpacing(10);
    m_filterFormLayout->setLabelAlignment(Qt::AlignLeft);
    m_filterFormLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    QString labelStyle = QString("color: %1; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK);
    QString inputStyle = QString("QComboBox, QSpinBox, QDateEdit, QLineEdit, QListWidget {"
                                 "   background-color: %1;"
                                 "   color: %2;"
                                 "   border: 1px solid %3;"
                                 "   border-radius: 5px;"
                                 "   padding: 5px;"
                                 "}"
                                 "QComboBox::drop-down {"
                                 "   subcontrol-origin: padding;"
                                 "   subcontrol-position: top right;"
                                 "   width: 20px;"
                                 "   border-left-width: 1px;"
                                 "   border-left-color: %3;"
                                 "   border-left-style: solid;"
                                 "   border-top-right-radius: 3px;"
                                 "   border-bottom-right-radius: 3px;"
                                 "}"
                                 "QComboBox::down-arrow {"
                                 "   image: url(:/icons/down_arrow.png);"
                                 "   width: 12px;"
                                 "   height: 12px;"
                                 "   left: 2px;"
                                 "}"
                                 "QSpinBox::up-button, QSpinBox::down-button {"
                                 "   background-color: %1;"
                                 "   width: 20px;"
                                 "   border: 1px solid %3;"
                                 "   border-radius: 3px;"
                                 "   subcontrol-origin: border;"
                                 "   subcontrol-position: top right;"
                                 "}"
                                 "QSpinBox::up-arrow {"
                                 "   image: url(:/icons/up_arrow.png);"
                                 "   width: 12px;"
                                 "   height: 12px;"
                                 "}"
                                 "QSpinBox::down-arrow {"
                                 "   image: url(:/icons/down_arrow.png);"
                                 "   width: 12px;"
                                 "   height: 12px;"
                                 "}"
                                 "QLineEdit {"
                                 "   selection-background-color: %4;"
                                 "}"
                                 "QCheckBox::indicator {"
                                 "   width: 18px;"
                                 "   height: 18px;"
                                 "}"
                                 "QCheckBox::indicator:unchecked {"
                                 "   image: url(:/icons/checkbox_unchecked.png);"
                                 "}"
                                 "QCheckBox::indicator:checked {"
                                 "   image: url(:/icons/checkbox_checked.png);"
                                 "}"
                                 "QCheckBox {"
                                 "   color: %2; spacing: 5px;"
                                 "}"
                                 "QListWidget {"
                                 "   selection-background-color: %4;"
                                 "   outline: none;"
                                 "}"
                                 "QListWidget::item {"
                                 "   padding: 3px;"
                                 "   color: %2;"
                                 "}"
                                 "QListWidget::item:selected {"
                                 "   background-color: %4;"
                                 "}"
                                 "QListWidget::item:hover {"
                                 "   background-color: %5;"
                                 "}"
                                 "QComboBox QAbstractItemView {"
                                 "   background-color: %1;"
                                 "   color: %2;"
                                 "   selection-background-color: %4;"
                                 "   border: 1px solid %3;"
                                 "   outline: none;"
                                 "}")
                             .arg(AppTheme::INPUT_BG,
                                  AppTheme::TEXT_ON_DARK,
                                  AppTheme::INPUT_BORDER,
                                  AppTheme::PRIMARY_RED_ALPHA_30,
                                  AppTheme::INTERACTIVE_HOVER_BG);

    QLabel *typeLabel = new QLabel("Type:", m_filterContainerWidget);
    typeLabel->setStyleSheet(labelStyle);
    m_typeCombo = new QComboBox(m_filterContainerWidget);
    m_typeCombo->addItems({"movie", "tv", "both"});
    m_typeCombo->setCurrentText("both");
    m_typeCombo->setStyleSheet(inputStyle);
    m_filterFormLayout->addRow(typeLabel, m_typeCombo);
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, [this](const QString &type) {
        m_currentType = type;
        fetchGenres(type);
        resetPagination();
    });
    m_currentType = m_typeCombo->currentText();

    QLabel *sortLabel = new QLabel("Sort By:", m_filterContainerWidget);
    sortLabel->setStyleSheet(labelStyle);
    m_sortCombo = new QComboBox(m_filterContainerWidget);
    m_sortCombo->setStyleSheet(inputStyle);
    m_sortOptionsMap["Popularity Descending"] = "popularity.desc";
    m_sortOptionsMap["Popularity Ascending"] = "popularity.asc";
    m_sortOptionsMap["Release Date Descending"] = "primary_release_date.desc";
    m_sortOptionsMap["Release Date Ascending"] = "primary_release_date.asc";
    m_sortOptionsMap["Vote Average Descending"] = "vote_average.desc";
    m_sortOptionsMap["Vote Average Ascending"] = "vote_average.asc";
    m_sortOptionsMap["Revenue Descending"] = "revenue.desc";
    m_sortOptionsMap["Revenue Ascending"] = "revenue.asc";
    for (const QString &humanReadable : m_sortOptionsMap.keys()) {
        m_sortCombo->addItem(humanReadable, m_sortOptionsMap.value(humanReadable));
    }
    // FIX: Set default sort to most popular
    m_sortCombo->setCurrentText("Popularity Descending");
    m_filterFormLayout->addRow(sortLabel, m_sortCombo);

    QLabel *yearLabel = new QLabel("Year:", m_filterContainerWidget);
    yearLabel->setStyleSheet(labelStyle);
    m_yearCombo = new QComboBox(m_filterContainerWidget);
    m_yearCombo->addItem("Any");
    int currentYear = QDate::currentDate().year();
    for (int y = currentYear; y >= currentYear - 100; --y) {
        m_yearCombo->addItem(QString::number(y));
    }
    m_yearCombo->setStyleSheet(inputStyle);
    m_filterFormLayout->addRow(yearLabel, m_yearCombo);

    auto createListWithClearButton = [this, labelStyle, inputStyle](QListWidget *&listWidget,
                                                                    const QString &labelText) {
        listWidget = new QListWidget(m_filterContainerWidget);
        listWidget->setSelectionMode(QAbstractItemView::MultiSelection);
        listWidget->setMinimumHeight(120);
        listWidget->setStyleSheet(inputStyle);

        QPushButton *clearButton = new QPushButton("Clear", m_filterContainerWidget);
        clearButton->setStyleSheet(
            QString("QPushButton {background-color:%1; color:%2; border:none; border-radius:5px; "
                    "padding: 5px 10px; font-weight:bold; font-size:12px;}"
                    "QPushButton:hover {background-color:%3;}")
                .arg(AppTheme::BUTTON_SECONDARY_BG,
                     AppTheme::TEXT_ON_DARK,
                     AppTheme::BUTTON_SECONDARY_HOVER_BG));
        connect(clearButton, &QPushButton::clicked, this, [this, listWidget]() {
            clearListWidgetSelection(listWidget);
        });

        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(listWidget, 4);
        layout->addWidget(clearButton, 1, Qt::AlignTop);

        QLabel *label = new QLabel(labelText, m_filterContainerWidget);
        label->setStyleSheet(labelStyle);
        m_filterFormLayout->addRow(label, layout);
    };

    createListWithClearButton(m_genreListWidget, "Genres:");
    createListWithClearButton(m_languageListWidget, "Languages:");
    createListWithClearButton(m_regionListWidget, "Regions:");

    QLabel *voteAvgLabel = new QLabel("Vote Average:", m_filterContainerWidget);
    voteAvgLabel->setStyleSheet(labelStyle);
    m_minVoteAverageSpinBox = new QSpinBox(m_filterContainerWidget);
    m_minVoteAverageSpinBox->setRange(0, 10);
    m_minVoteAverageSpinBox->setValue(0);
    m_minVoteAverageSpinBox->setStyleSheet(inputStyle);
    m_maxVoteAverageSpinBox = new QSpinBox(m_filterContainerWidget);
    m_maxVoteAverageSpinBox->setRange(0, 10);
    m_maxVoteAverageSpinBox->setValue(10);
    m_maxVoteAverageSpinBox->setStyleSheet(inputStyle);
    QHBoxLayout *voteAvgLayout = new QHBoxLayout();
    voteAvgLayout->addWidget(m_minVoteAverageSpinBox);
    voteAvgLayout->addWidget(new QLabel(" to ", m_filterContainerWidget));
    voteAvgLayout->addWidget(m_maxVoteAverageSpinBox);
    m_filterFormLayout->addRow(voteAvgLabel, voteAvgLayout);

    QLabel *voteCountLabel = new QLabel("Vote Count:", m_filterContainerWidget);
    voteCountLabel->setStyleSheet(labelStyle);
    m_minVoteCountSpinBox = new QSpinBox(m_filterContainerWidget);
    m_minVoteCountSpinBox->setRange(0, 1000000);
    m_minVoteCountSpinBox->setValue(0);
    m_minVoteCountSpinBox->setSingleStep(1000);
    m_minVoteCountSpinBox->setStyleSheet(inputStyle);
    m_maxVoteCountSpinBox = new QSpinBox(m_filterContainerWidget);
    m_maxVoteCountSpinBox->setRange(0, 1000000);
    m_maxVoteCountSpinBox->setValue(1000000);
    m_maxVoteCountSpinBox->setSingleStep(10000);
    m_maxVoteCountSpinBox->setStyleSheet(inputStyle);
    QHBoxLayout *voteCountLayout = new QHBoxLayout();
    voteCountLayout->addWidget(m_minVoteCountSpinBox);
    voteCountLayout->addWidget(new QLabel(" to ", m_filterContainerWidget));
    voteCountLayout->addWidget(m_maxVoteCountSpinBox);
    m_filterFormLayout->addRow(voteCountLabel, voteCountLayout);

    QLabel *releaseDateLabel = new QLabel("Release Date:", m_filterContainerWidget);
    releaseDateLabel->setStyleSheet(labelStyle);
    m_releaseDateFromEdit = new QDateEdit(m_filterContainerWidget);
    m_releaseDateFromEdit->setCalendarPopup(true);
    m_releaseDateFromEdit->setDate(QDate(1900, 1, 1));
    m_releaseDateFromEdit->setStyleSheet(inputStyle);
    m_releaseDateToEdit = new QDateEdit(m_filterContainerWidget);
    m_releaseDateToEdit->setCalendarPopup(true);
    m_releaseDateToEdit->setDate(QDate::currentDate());
    m_releaseDateToEdit->setStyleSheet(inputStyle);
    QHBoxLayout *releaseDateLayout = new QHBoxLayout();
    releaseDateLayout->addWidget(m_releaseDateFromEdit);
    releaseDateLayout->addWidget(new QLabel(" to ", m_filterContainerWidget));
    releaseDateLayout->addWidget(m_releaseDateToEdit);
    m_filterFormLayout->addRow(releaseDateLabel, releaseDateLayout);

    QLabel *runtimeLabel = new QLabel("Runtime:", m_filterContainerWidget);
    runtimeLabel->setStyleSheet(labelStyle);
    m_minRuntimeSpinBox = new QSpinBox(m_filterContainerWidget);
    m_minRuntimeSpinBox->setRange(0, 1000);
    m_minRuntimeSpinBox->setValue(0);
    m_minRuntimeSpinBox->setSuffix(" min");
    m_minRuntimeSpinBox->setStyleSheet(inputStyle);
    m_maxRuntimeSpinBox = new QSpinBox(m_filterContainerWidget);
    m_maxRuntimeSpinBox->setRange(0, 1000);
    m_maxRuntimeSpinBox->setValue(500);
    m_maxRuntimeSpinBox->setSuffix(" min");
    m_maxRuntimeSpinBox->setStyleSheet(inputStyle);
    QHBoxLayout *runtimeLayout = new QHBoxLayout();
    runtimeLayout->addWidget(m_minRuntimeSpinBox);
    runtimeLayout->addWidget(new QLabel(" to ", m_filterContainerWidget));
    runtimeLayout->addWidget(m_maxRuntimeSpinBox);
    m_filterFormLayout->addRow(runtimeLabel, runtimeLayout);

    m_includeAdultCheckBox = new QCheckBox("Include Adult Content", m_filterContainerWidget);
    m_includeAdultCheckBox->setStyleSheet(
        QString("color: %1; spacing: 5px;").arg(AppTheme::TEXT_ON_DARK));
    m_filterFormLayout->addRow(m_includeAdultCheckBox);

    QLabel *keywordsLabel = new QLabel("Keywords:", m_filterContainerWidget);
    keywordsLabel->setStyleSheet(labelStyle);
    m_keywordsLineEdit = new QLineEdit(m_filterContainerWidget);
    m_keywordsLineEdit->setPlaceholderText("Enter keywords (comma-separated IDs)");
    m_keywordsLineEdit->setStyleSheet(inputStyle);
    m_filterFormLayout->addRow(keywordsLabel, m_keywordsLineEdit);

    m_contentVBoxLayout->addWidget(m_filterContainerWidget);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    m_applyButton = new QPushButton("Apply Filters", m_scrollContent);
    m_applyButton->setStyleSheet(QString("QPushButton {"
                                         "   background-color: %1;"
                                         "   color: %2;"
                                         "   border: none;"
                                         "   border-radius: 8px;"
                                         "   padding: 12px 24px;"
                                         "   font-weight: bold;"
                                         "}"
                                         "QPushButton:hover {"
                                         "   background-color: %3;"
                                         "}"
                                         "QPushButton:pressed {"
                                         "   background-color: %4;"
                                         "}"
                                         "QPushButton:disabled {"
                                         "   background-color: %5;"
                                         "   color: %6;"
                                         "}")
                                     .arg(AppTheme::PRIMARY_RED,
                                          AppTheme::TEXT_ON_DARK,
                                          AppTheme::BUTTON_HOVER_DARK_BG,
                                          AppTheme::DARK_RED,
                                          AppTheme::BUTTON_DISABLED_BG,
                                          AppTheme::TEXT_ON_DARK_SECONDARY));

    m_clearButton = new QPushButton("Clear All Filters", m_scrollContent);
    m_clearButton->setStyleSheet(QString("QPushButton {"
                                         "   background-color: %1;"
                                         "   color: %2;"
                                         "   border: none;"
                                         "   border-radius: 8px;"
                                         "   padding: 12px 24px;"
                                         "   font-weight: bold;"
                                         "}"
                                         "QPushButton:hover {"
                                         "   background-color: %3;"
                                         "}"
                                         "QPushButton:pressed {"
                                         "   background-color: %4;"
                                         "}")
                                     .arg(AppTheme::BUTTON_SECONDARY_BG,
                                          AppTheme::TEXT_ON_DARK,
                                          AppTheme::BUTTON_SECONDARY_HOVER_BG,
                                          AppTheme::BUTTON_SECONDARY_PRESSED_BG));

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_applyButton);
    buttonsLayout->addWidget(m_clearButton);
    buttonsLayout->addStretch();
    m_contentVBoxLayout->addLayout(buttonsLayout);

    m_gridLayout = new QGridLayout();
    m_gridLayout->setSpacing(20);
    m_gridLayout->setContentsMargins(10, 10, 10, 10);
    m_contentVBoxLayout->addLayout(m_gridLayout);

    m_statusLabel = new QLabel(m_scrollContent);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->hide();

    connect(m_applyButton, &QPushButton::clicked, this, [this]() { applyFilters(true); });
    connect(m_clearButton, &QPushButton::clicked, this, &CategoryFilterWidget::clearFilters);
    connect(m_scrollArea->verticalScrollBar(),
            &QScrollBar::valueChanged,
            this,
            &CategoryFilterWidget::onScroll);
}

void CategoryFilterWidget::toggleFilterVisibility()
{
    m_filtersVisible = !m_filtersVisible;
    m_filterContainerWidget->setVisible(m_filtersVisible);
    m_filterToggleButton->setText(m_filtersVisible ? "Hide Filters" : "Show Filters");
}

void CategoryFilterWidget::loadFilters()
{
    fetchGenres(m_currentType);
    fetchLanguages();
    fetchRegions();
    applyFilters(true);
}

void CategoryFilterWidget::applyFilters(bool reset)
{
    if (m_fetching)
        return;

    if (reset) {
        resetPagination();
    } else {
        ++m_currentPage;
    }

    m_fetching = true;
    m_applyButton->setEnabled(false);
    m_applyButton->setText("Loading...");

    if (m_currentPage == 1) {
        showStatusMessage("Loading results...");
    }

    QString apiType = m_currentType.toLower();
    QString baseUrl;

    if (apiType == "both") {
        baseUrl = QString("https://api.themoviedb.org/3/discover/movie");
        QUrl url(baseUrl);
        QUrlQuery query;
        query.addQueryItem("api_key", "b4c5ef5419f9f4ce8a627faa1e2be530");
        query.addQueryItem("language", "en-US");

        QString selectedSortKey = m_sortCombo->currentText();
        QString apiSortValue = m_sortOptionsMap.value(selectedSortKey, "popularity.desc");
        query.addQueryItem("sort_by", apiSortValue);

        query.addQueryItem("page", QString::number(m_currentPage));
        query.addQueryItem("include_adult", m_includeAdultCheckBox->isChecked() ? "true" : "false");

        if (m_yearCombo->currentText() != "Any") {
            query.addQueryItem("primary_release_year", m_yearCombo->currentText());
        }

        QStringList genres = selectedGenreIds();
        if (!genres.isEmpty())
            query.addQueryItem("with_genres", genres.join(","));

        QStringList langs = selectedLanguageCodes();
        if (!langs.isEmpty())
            query.addQueryItem("with_original_language", langs.join("|"));

        QStringList regions = selectedRegionCodes();
        if (!regions.isEmpty())
            query.addQueryItem("region", regions.join(","));

        int minVoteAvg = m_minVoteAverageSpinBox->value();
        int maxVoteAvg = m_maxVoteAverageSpinBox->value();
        if (minVoteAvg > 0)
            query.addQueryItem("vote_average.gte", QString::number(minVoteAvg));
        if (maxVoteAvg < 10)
            query.addQueryItem("vote_average.lte", QString::number(maxVoteAvg));

        int minVoteCount = m_minVoteCountSpinBox->value();
        int maxVoteCount = m_maxVoteCountSpinBox->value();
        if (minVoteCount > 0)
            query.addQueryItem("vote_count.gte", QString::number(minVoteCount));
        if (maxVoteCount < 1000000)
            query.addQueryItem("vote_count.lte", QString::number(maxVoteCount));

        if (m_releaseDateFromEdit->date().isValid()
            && m_releaseDateFromEdit->date() > QDate(1900, 1, 1)) {
            query.addQueryItem("primary_release_date.gte",
                               m_releaseDateFromEdit->date().toString("yyyy-MM-dd"));
        }
        if (m_releaseDateToEdit->date().isValid()
            && m_releaseDateToEdit->date() < QDate::currentDate()) {
            query.addQueryItem("primary_release_date.lte",
                               m_releaseDateToEdit->date().toString("yyyy-MM-dd"));
        }

        int minRuntime = m_minRuntimeSpinBox->value();
        int maxRuntime = m_maxRuntimeSpinBox->value();
        if (minRuntime > 0)
            query.addQueryItem("with_runtime.gte", QString::number(minRuntime));
        if (maxRuntime > 0 && maxRuntime < 1000)
            query.addQueryItem("with_runtime.lte", QString::number(maxRuntime));

        if (!m_keywordsLineEdit->text().isEmpty()) {
            query.addQueryItem("with_keywords", m_keywordsLineEdit->text());
        }

        url.setQuery(query);
        qDebug() << "Fetching URL:" << url.toString();

        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            processMoviesReply(reply);
            reply->deleteLater();
        });

    } else if (apiType == "movie" || apiType == "tv") {
        baseUrl = QString("https://api.themoviedb.org/3/discover/%1").arg(apiType);
        QUrl url(baseUrl);
        QUrlQuery query;
        query.addQueryItem("api_key", "b4c5ef5419f9f4ce8a627faa1e2be530");
        query.addQueryItem("language", "en-US");

        QString selectedSortKey = m_sortCombo->currentText();
        QString apiSortValue = m_sortOptionsMap.value(selectedSortKey, "popularity.desc");
        query.addQueryItem("sort_by", apiSortValue);

        query.addQueryItem("page", QString::number(m_currentPage));
        query.addQueryItem("include_adult", m_includeAdultCheckBox->isChecked() ? "true" : "false");

        if (m_yearCombo->currentText() != "Any") {
            if (apiType == "movie")
                query.addQueryItem("primary_release_year", m_yearCombo->currentText());
            else if (apiType == "tv")
                query.addQueryItem("first_air_date_year", m_yearCombo->currentText());
        }

        QStringList genres = selectedGenreIds();
        if (!genres.isEmpty())
            query.addQueryItem("with_genres", genres.join(","));

        QStringList langs = selectedLanguageCodes();
        if (!langs.isEmpty())
            query.addQueryItem("with_original_language", langs.join("|"));

        QStringList regions = selectedRegionCodes();
        if (!regions.isEmpty())
            query.addQueryItem("region", regions.join(","));

        int minVoteAvg = m_minVoteAverageSpinBox->value();
        int maxVoteAvg = m_maxVoteAverageSpinBox->value();
        if (minVoteAvg > 0)
            query.addQueryItem("vote_average.gte", QString::number(minVoteAvg));
        if (maxVoteAvg < 10)
            query.addQueryItem("vote_average.lte", QString::number(maxVoteAvg));

        int minVoteCount = m_minVoteCountSpinBox->value();
        int maxVoteCount = m_maxVoteCountSpinBox->value();
        if (minVoteCount > 0)
            query.addQueryItem("vote_count.gte", QString::number(minVoteCount));
        if (maxVoteCount < 1000000)
            query.addQueryItem("vote_count.lte", QString::number(maxVoteCount));

        if (m_releaseDateFromEdit->date().isValid()
            && m_releaseDateFromEdit->date() > QDate(1900, 1, 1)) {
            QString fromDate = m_releaseDateFromEdit->date().toString("yyyy-MM-dd");
            if (apiType == "movie") {
                query.addQueryItem("primary_release_date.gte", fromDate);
            } else {
                query.addQueryItem("first_air_date.gte", fromDate);
            }
        }
        if (m_releaseDateToEdit->date().isValid()
            && m_releaseDateToEdit->date() < QDate::currentDate()) {
            QString toDate = m_releaseDateToEdit->date().toString("yyyy-MM-dd");
            if (apiType == "movie") {
                query.addQueryItem("primary_release_date.lte", toDate);
            } else { // tv
                query.addQueryItem("first_air_date.lte", toDate);
            }
        }

        int minRuntime = m_minRuntimeSpinBox->value();
        int maxRuntime = m_maxRuntimeSpinBox->value();
        if (minRuntime > 0)
            query.addQueryItem("with_runtime.gte", QString::number(minRuntime));
        if (maxRuntime > 0 && maxRuntime < 1000)
            query.addQueryItem("with_runtime.lte", QString::number(maxRuntime));

        if (!m_keywordsLineEdit->text().isEmpty()) {
            query.addQueryItem("with_keywords", m_keywordsLineEdit->text());
        }

        url.setQuery(query);
        qDebug() << "Fetching URL:" << url.toString();

        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            processMoviesReply(reply);
            reply->deleteLater();
        });
    } else {
        qWarning() << "Unsupported type for discovery API:" << apiType;
        m_fetching = false;
        m_applyButton->setEnabled(true);
        m_applyButton->setText("Apply Filters");
        showStatusMessage(QString("Unsupported type: %1").arg(apiType), AppTheme::PRIMARY_RED);
        return;
    }
}

void CategoryFilterWidget::processMoviesReply(QNetworkReply *reply)
{
    m_fetching = false;
    m_applyButton->setEnabled(true);
    m_applyButton->setText("Apply Filters");

    if (reply->error()) {
        qWarning() << "Movies/TV fetch error:" << reply->errorString();
        if (m_currentPage == 1) {
            showStatusMessage("Error: " + reply->errorString(), AppTheme::PRIMARY_RED);
        }
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON response for movies/TV.";
        if (m_currentPage == 1) {
            showStatusMessage("No valid data received.", AppTheme::TEXT_ON_DARK_SECONDARY);
        }
        return;
    }

    QJsonArray results = doc.object()["results"].toArray();

    if (results.isEmpty() && m_currentPage == 1) {
        showStatusMessage("No results found for the selected filters.");
        return;
    }

    m_statusLabel->hide();

    QLayoutItem *lastItem = m_gridLayout->itemAt(m_gridLayout->count() - 1);
    if (lastItem && lastItem->widget() && qobject_cast<QLabel *>(lastItem->widget())
        && qobject_cast<QLabel *>(lastItem->widget())->text().contains("Loading more")) {
        m_gridLayout->removeWidget(lastItem->widget());
        lastItem->widget()->deleteLater();
        delete lastItem;
    }

    int row = m_gridLayout->rowCount();
    int col = 0;
    if (row > 0 && m_gridLayout->itemAtPosition(row - 1, m_gridColumns - 1) != nullptr) {
        row = m_gridLayout->rowCount();
        col = 0;
    } else if (row > 0) {
        for (int c = 0; c < m_gridColumns; ++c) {
            if (m_gridLayout->itemAtPosition(row - 1, c) == nullptr) {
                col = c;
                row = row - 1;
                break;
            }
        }
    }

    for (const auto &val : results) {
        QJsonObject contentObj = val.toObject();
        MovieCard *card = new MovieCard(contentObj,
                                        "https://image.tmdb.org/t/p/",
                                        "w342",
                                        m_scrollContent);
        connect(card, &MovieCard::clicked, this, [this, contentObj]() {
            int id = contentObj["id"].toInt();
            QString type;
            if (contentObj.contains("title") && !contentObj.contains("name")) {
                type = "movie";
            } else if (contentObj.contains("name")) {
                type = "tv";
            } else {
                type = m_currentType;
            }
            emit movieClicked(id, type);
        });
        m_movieCards.append(card);
        m_gridLayout->addWidget(card, row, col);
        col++;
        if (col >= m_gridColumns) {
            col = 0;
            row++;
        }
    }
    m_scrollContent->adjustSize();
}

void CategoryFilterWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int cardWidthEstimate = 200 + m_gridLayout->spacing();
    if (m_scrollContent->width() <= 0)
        return;

    int newColumns = qMax(4, m_scrollContent->width() / cardWidthEstimate);

    if (newColumns != m_gridColumns) {
        m_gridColumns = newColumns;
        relayoutGrid();
    }
}

void CategoryFilterWidget::relayoutGrid()
{
    QList<QWidget*> cardWidgets;
    for(MovieCard* card : m_movieCards) {
        cardWidgets.append(card);
    }

    for (QWidget *widget : cardWidgets) {
        m_gridLayout->removeWidget(widget);
    }

    int row = 0, col = 0;
    for (QWidget *widget : cardWidgets) {
        if (widget) {
            m_gridLayout->addWidget(widget, row, col);
            col++;
            if (col >= m_gridColumns) {
                col = 0;
                row++;
            }
        }
    }

    m_statusLabel->hide();
    if (m_movieCards.isEmpty() && m_currentPage == 1) {
        showStatusMessage("No results found for the selected filters.");
    }
}

void CategoryFilterWidget::clearGrid(bool deleteWidgets)
{
    m_statusLabel->hide();

    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
            if (deleteWidgets && child->widget() != m_statusLabel) {
                child->widget()->deleteLater();
            }
        }
        delete child;
    }

    if (deleteWidgets) {
        m_movieCards.clear();
    }
}

void CategoryFilterWidget::showStatusMessage(const QString &message, const QString &color)
{
    clearGrid(true);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(
        QString("color: %1; font-size: 16px; font-weight: bold;").arg(color));
    m_statusLabel->show();
    m_gridLayout->addWidget(m_statusLabel, 0, 0, 1, m_gridColumns, Qt::AlignCenter);
}

void CategoryFilterWidget::resetPagination()
{
    m_currentPage = 1;
    clearGrid(true);
}

void CategoryFilterWidget::clearFilters()
{
    m_typeCombo->setCurrentText("both");
    m_sortCombo->setCurrentText("Popularity Descending");
    m_yearCombo->setCurrentIndex(0);

    clearListWidgetSelection(m_genreListWidget);
    clearListWidgetSelection(m_languageListWidget);
    clearListWidgetSelection(m_regionListWidget);

    m_minVoteAverageSpinBox->setValue(0);
    m_maxVoteAverageSpinBox->setValue(10);
    m_minVoteCountSpinBox->setValue(0);
    m_maxVoteCountSpinBox->setValue(1000000);
    m_releaseDateFromEdit->setDate(QDate(1900, 1, 1));
    m_releaseDateToEdit->setDate(QDate::currentDate());
    m_minRuntimeSpinBox->setValue(0);
    m_maxRuntimeSpinBox->setValue(500);
    m_includeAdultCheckBox->setChecked(false);
    m_keywordsLineEdit->clear();

    resetPagination();
    applyFilters(true);
}

void CategoryFilterWidget::clearListWidgetSelection(QListWidget *listWidget)
{
    listWidget->clearSelection();
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        if (item->flags() & Qt::ItemIsUserCheckable) {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

void CategoryFilterWidget::onScroll()
{
    QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
    if (scrollBar->value() >= scrollBar->maximum() * 0.9 && !m_fetching) {
        applyFilters(false);
    }
}

void CategoryFilterWidget::fetchGenres(const QString &type)
{
    if (!m_networkManager)
        return;

    QString apiType = type.toLower();

    if (apiType == "both") {
        apiType = "movie";
    }

    if (apiType == "person") {
        m_genreMap.clear();
        m_genreListWidget->clear();
        QListWidgetItem *item = new QListWidgetItem("N/A for People", m_genreListWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsUserCheckable);
        item->setForeground(QBrush(QColor(AppTheme::TEXT_ON_DARK_SECONDARY)));
        qDebug() << "Genre fetching skipped for 'Person' type.";
        return;
    }

    QString url = QString("https://api.themoviedb.org/3/genre/%1/"
                          "list?api_key=b4c5ef5419f9f4ce8a627faa1e2be530&language=en-US")
                      .arg(apiType);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processGenreReply(reply);
        reply->deleteLater();
    });
}

void CategoryFilterWidget::processGenreReply(QNetworkReply *reply)
{
    if (reply->error()) {
        qWarning() << "Genre fetch error:" << reply->errorString();
        m_genreListWidget->clear();
        QListWidgetItem *item = new QListWidgetItem("Error loading genres", m_genreListWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsUserCheckable);
        item->setForeground(QBrush(QColor(AppTheme::PRIMARY_RED)));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON response for genres.";
        return;
    }

    QJsonObject obj = doc.object();
    QJsonArray genres = obj["genres"].toArray();

    m_genreMap.clear();
    m_genreListWidget->clear();

    if (genres.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No genres available", m_genreListWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        item->setForeground(QBrush(QColor(AppTheme::TEXT_ON_DARK_SECONDARY)));
        return;
    }

    for (const auto &genreVal : genres) {
        auto genreObj = genreVal.toObject();
        QString id = QString::number(genreObj["id"].toInt());
        QString name = genreObj["name"].toString();
        m_genreMap[id] = name;

        QListWidgetItem *item = new QListWidgetItem(name, m_genreListWidget);
        item->setData(Qt::UserRole, id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void CategoryFilterWidget::fetchLanguages()
{
    if (!m_networkManager)
        return;

    QString url = QString("https://api.themoviedb.org/3/configuration/"
                          "languages?api_key=b4c5ef5419f9f4ce8a627faa1e2be530");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processLanguagesReply(reply);
        reply->deleteLater();
    });
}

void CategoryFilterWidget::processLanguagesReply(QNetworkReply *reply)
{
    if (reply->error()) {
        qWarning() << "Languages fetch error:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid JSON response for languages.";
        return;
    }

    QJsonArray array = doc.array();

    m_languageMap.clear();
    m_languageListWidget->clear();

    if (array.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No languages available", m_languageListWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        item->setForeground(QBrush(QColor(AppTheme::TEXT_ON_DARK_SECONDARY)));
        return;
    }

    for (const auto &val : array) {
        QJsonObject obj = val.toObject();
        QString code = obj["iso_639_1"].toString();
        QString name = obj["english_name"].toString();
        m_languageMap[code] = name;

        QListWidgetItem *item = new QListWidgetItem(name, m_languageListWidget);
        item->setData(Qt::UserRole, code);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void CategoryFilterWidget::fetchRegions()
{
    if (!m_networkManager)
        return;

    QString url = QString("https://api.themoviedb.org/3/configuration/"
                          "countries?api_key=b4c5ef5419f9f4ce8a627faa1e2be530");
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processRegionsReply(reply);
        reply->deleteLater();
    });
}

void CategoryFilterWidget::processRegionsReply(QNetworkReply *reply)
{
    if (reply->error()) {
        qWarning() << "Regions fetch error:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid JSON response for regions.";
        return;
    }

    QJsonArray array = doc.array();

    m_regionMap.clear();
    m_regionListWidget->clear();

    if (array.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No regions available", m_regionListWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        item->setForeground(QBrush(QColor(AppTheme::TEXT_ON_DARK_SECONDARY)));
        return;
    }

    for (const auto &val : array) {
        QJsonObject obj = val.toObject();
        QString code = obj["iso_3166_1"].toString();
        QString name = obj["english_name"].toString();
        m_regionMap[code] = name;

        QListWidgetItem *item = new QListWidgetItem(name, m_regionListWidget);
        item->setData(Qt::UserRole, code);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

QStringList CategoryFilterWidget::selectedGenreIds() const
{
    QStringList ids;
    for (int i = 0; i < m_genreListWidget->count(); ++i) {
        QListWidgetItem *item = m_genreListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            ids << item->data(Qt::UserRole).toString();
    }
    return ids;
}

QStringList CategoryFilterWidget::selectedLanguageCodes() const
{
    QStringList codes;
    for (int i = 0; i < m_languageListWidget->count(); ++i) {
        QListWidgetItem *item = m_languageListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            codes << item->data(Qt::UserRole).toString();
    }
    return codes;
}

QStringList CategoryFilterWidget::selectedRegionCodes() const
{
    QStringList codes;
    for (int i = 0; i < m_regionListWidget->count(); ++i) {
        QListWidgetItem *item = m_regionListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            codes << item->data(Qt::UserRole).toString();
    }
    return codes;
}
