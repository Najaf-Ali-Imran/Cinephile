#include "WatchPage.h"
#include "MovieCard.h"
#include "Theme.h"

#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedLayout>
#include <QUrlQuery>
#include <QVBoxLayout>

WatchPage::WatchPage(QNetworkAccessManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
    , m_gridColumns(5)
{
    m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";
    if (!m_networkManager) {
        qFatal("CRITICAL: WatchPage created with a null QNetworkAccessManager.");
    }
    setupUi();
    showStatusMessage("Search for movies to discover your next watch",
                      AppTheme::TEXT_ON_DARK_SECONDARY);
}

WatchPage::~WatchPage()
{
}

void WatchPage::setupUi()
{
    this->setObjectName("watchPage");
    this->setStyleSheet(
        QString("QWidget#watchPage { background-color: %1; }").arg(AppTheme::MAIN_WINDOW_BG));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *headerContainer = new QWidget(this);
    headerContainer->setFixedHeight(200);
    headerContainer->setStyleSheet(
        QString("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);")
            .arg(AppTheme::MAIN_WINDOW_BG, "rgba(0,0,0,0.3)"));

    QVBoxLayout *headerLayout = new QVBoxLayout(headerContainer);
    headerLayout->setContentsMargins(40, 30, 40, 30);
    headerLayout->setSpacing(8);

    QWidget *titleContainer = new QWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(12);

    QLabel *iconPlaceholder = new QLabel();
    iconPlaceholder->setFixedSize(48, 48);
    iconPlaceholder->setStyleSheet(
        QString("background-color: %1; border-radius: 12px; border: 2px solid %2;")
            .arg(AppTheme::PRIMARY_RED, "rgba(255,255,255,0.1)"));
    iconPlaceholder->setText("🎬");
    iconPlaceholder->setAlignment(Qt::AlignCenter);
    iconPlaceholder->setStyleSheet(iconPlaceholder->styleSheet() + " font-size: 24px;");

    QLabel *titleLabel = new QLabel("Watch Movies");
    titleLabel->setStyleSheet(
        QString("color: %1; font-size: 32px; font-weight: 700; letter-spacing: -1px;")
            .arg(AppTheme::TEXT_ON_DARK));

    titleLayout->addWidget(iconPlaceholder);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QLabel *subtitleLabel = new QLabel("Discover and stream");
    subtitleLabel->setStyleSheet(
        QString("color: %1; font-size: 16px; font-weight: 400; margin-top: 4px;")
            .arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    headerLayout->addWidget(titleContainer);
    headerLayout->addWidget(subtitleLabel);
    headerLayout->addStretch();

    QWidget *searchContainer = new QWidget();
    searchContainer->setFixedHeight(70);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(12);

    m_searchBar = new QLineEdit();
    m_searchBar->setPlaceholderText("Search movies, genres, or actors...");
    m_searchBar->setFixedHeight(54);
    m_searchBar->setStyleSheet(
        QString("QLineEdit {"
                "    background-color: rgba(255,255,255,0.08);"
                "    color: %1;"
                "    border: 2px solid rgba(255,255,255,0.12);"
                "    border-radius: 16px;"
                "    padding: 0 20px 0 50px;"
                "    font-size: 16px;"
                "    font-weight: 500;"
                "}"
                "QLineEdit:focus {"
                "    border: 2px solid %2;"
                "    background-color: rgba(255,255,255,0.12);"
                "    outline: none;"
                "}"
                "QLineEdit::placeholder {"
                "    color: %3;"
                "}")
            .arg(AppTheme::TEXT_ON_DARK, AppTheme::PRIMARY_RED, AppTheme::TEXT_ON_DARK_SECONDARY));

    connect(m_searchBar, &QLineEdit::returnPressed, this, &WatchPage::onSearchSubmitted);

    QLabel *searchIcon = new QLabel(m_searchBar);
    searchIcon->setFixedSize(20, 20);
    searchIcon->setText("🔍");
    searchIcon->setStyleSheet(QString("color: %1; background: transparent; border: none;")
                                  .arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    searchIcon->move(18, 17);

    m_searchButton = new QPushButton("Search");
    m_searchButton->setFixedSize(120, 54);
    m_searchButton->setCursor(Qt::PointingHandCursor);
    m_searchButton->setStyleSheet(
        QString("QPushButton {"
                "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                "    color: %3;"
                "    border: none;"
                "    border-radius: 16px;"
                "    font-weight: 600;"
                "    font-size: 15px;"
                "    letter-spacing: 0.5px;"
                "}"
                "QPushButton:hover {"
                "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %4, stop:1 %5);"
                "    transform: translateY(-1px);"
                "}"
                "QPushButton:pressed {"
                "    background: %6;"
                "}")
            .arg(AppTheme::PRIMARY_RED,
                 "#b30000",
                 AppTheme::TEXT_ON_DARK,
                 "#ff4444",
                 "#cc0000",
                 "#990000"));

    connect(m_searchButton, &QPushButton::clicked, this, &WatchPage::onSearchSubmitted);

    searchLayout->addWidget(m_searchBar, 1);
    searchLayout->addWidget(m_searchButton);

    headerLayout->addWidget(searchContainer);
    mainLayout->addWidget(headerContainer);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(3);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        QString("QProgressBar {"
                "    background-color: rgba(255,255,255,0.1);"
                "    border: none;"
                "    border-radius: 1px;"
                "}"
                "QProgressBar::chunk {"
                "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2);"
                "    border-radius: 1px;"
                "}")
            .arg(AppTheme::PRIMARY_RED, "#ff6b6b"));
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);

    m_contentStack = new QStackedLayout();
    m_contentStack->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        QString("QScrollArea { background: transparent; border: none; }"
                "QScrollBar:vertical { background-color: rgba(255,255,255,0.05); width: 6px; "
                "border-radius: 3px; margin: 0; }"
                "QScrollBar::handle:vertical { background-color: rgba(255,255,255,0.2); "
                "border-radius: 3px; min-height: 30px; }"
                "QScrollBar::handle:vertical:hover { background-color: %1; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
            .arg(AppTheme::PRIMARY_RED));

    m_scrollContent = new QWidget();
    m_scrollContent->setMinimumWidth(1200);
    m_scrollContent->setStyleSheet("background: transparent;");

    QHBoxLayout *centeringLayout = new QHBoxLayout(m_scrollContent);
    centeringLayout->setContentsMargins(0, 0, 0, 0);
    centeringLayout->setSpacing(0);

    m_movieGrid = new QGridLayout();
    m_movieGrid->setContentsMargins(40, 30, 40, 30);
    m_movieGrid->setSpacing(24);
    m_movieGrid->setAlignment(Qt::AlignTop);

    centeringLayout->addStretch(1);
    centeringLayout->addLayout(m_movieGrid);
    centeringLayout->addStretch(1);

    m_scrollArea->setWidget(m_scrollContent);

    m_statusContainerWidget = new QWidget();
    m_statusContainerWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *statusContainerLayout = new QVBoxLayout(m_statusContainerWidget);
    statusContainerLayout->setContentsMargins(40, 30, 40, 30);
    statusContainerLayout->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    statusContainerLayout->addWidget(m_statusLabel);

    m_contentStack->addWidget(m_scrollArea);
    m_contentStack->addWidget(m_statusContainerWidget);

    mainLayout->addLayout(m_contentStack, 1);
}

void WatchPage::onSearchSubmitted()
{
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) {
        showStatusMessage("Please enter a search term to discover movies",
                          AppTheme::TEXT_ON_DARK_SECONDARY);
        return;
    }
    fetchAndDisplayMovies(query);
}

void WatchPage::fetchAndDisplayMovies(const QString &query)
{
    clearGrid();
    showStatusMessage(QString("Searching for '%1'...").arg(query), AppTheme::TEXT_ON_DARK_SECONDARY);
    m_progressBar->setRange(0, 0);
    m_progressBar->show();

    QUrl url("https://api.themoviedb.org/3/search/movie");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("api_key", m_apiKey);
    urlQuery.addQueryItem("language", "en-US");
    urlQuery.addQueryItem("query", query);
    urlQuery.addQueryItem("page", "1");
    urlQuery.addQueryItem("include_adult", "false");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processMovieReply(reply);
        reply->deleteLater();
    });
}

void WatchPage::processMovieReply(QNetworkReply *reply)
{
    m_progressBar->hide();
    m_progressBar->setRange(0, 100);

    if (!reply || reply->error() != QNetworkReply::NoError) {
        showStatusMessage(QString("Connection failed: %1")
                              .arg(reply ? reply->errorString() : "Unknown error"),
                              AppTheme::PRIMARY_RED);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray results = doc.object()["results"].toArray();

    clearGrid();

    if (results.isEmpty()) {
        showStatusMessage("No movies found matching your search", AppTheme::TEXT_ON_DARK_SECONDARY);
        return;
    }

    int row = 0, col = 0;
    for (const QJsonValue &value : results) {
        QJsonObject contentObj = value.toObject();
        if (contentObj["poster_path"].isNull() || contentObj["poster_path"].toString().isEmpty()) {
            continue;
        }

        MovieCard *card = new MovieCard(contentObj,
                                        "https://image.tmdb.org/t/p/",
                                        "w342",
                                        m_scrollContent);
        connect(card, &MovieCard::clicked, this, &WatchPage::onMovieCardClicked);
        m_movieCards.append(card);
        m_movieGrid->addWidget(card, row, col);

        col++;
        if (col >= m_gridColumns) {
            col = 0;
            row++;
        }
    }

    if (m_movieCards.isEmpty()) {
        showStatusMessage("No movies with available posters found",
                          AppTheme::TEXT_ON_DARK_SECONDARY);
    } else {
        showMovieGrid();
        relayoutGrid();
    }
}

void WatchPage::onMovieCardClicked(const QJsonObject &itemData)
{
    int tmdbId = itemData["id"].toInt();
    if (tmdbId <= 0) {
        QMessageBox::warning(this, "Error", "Invalid movie data. Cannot play this movie.");
        return;
    }

    QUrl url(QString("https://vidlink.pro/movie/%1").arg(tmdbId));
    QUrlQuery query;
    query.addQueryItem("primaryColor", "000000");
    query.addQueryItem("secondaryColor", "EEEBDd");
    query.addQueryItem("iconColor", "CE1212");
    query.addQueryItem("autoplay", "true");
    url.setQuery(query);

    QDesktopServices::openUrl(url);
}

void WatchPage::clearGrid()
{
    QLayoutItem *child;
    while ((child = m_movieGrid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_movieCards.clear();
}

void WatchPage::showStatusMessage(const QString &message, const QString &color)
{
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(
        QString("QLabel {"
                "    color: %1;"
                "    font-size: 18px;"
                "    font-weight: 500;"
                "    padding: 60px 40px;"
                "    border-radius: 20px;"
                "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                "        stop:0 rgba(255,255,255,0.03), "
                "        stop:1 rgba(255,255,255,0.01));"
                "    border: 1px solid rgba(255,255,255,0.08);"
                "}")
            .arg(color));

    m_contentStack->setCurrentWidget(m_statusContainerWidget);
}

void WatchPage::showMovieGrid()
{
    m_contentStack->setCurrentWidget(m_scrollArea);
}

void WatchPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayoutGrid();
}

void WatchPage::relayoutGrid()
{
    if (!m_scrollContent || !m_movieGrid || m_movieCards.isEmpty())
        return;

    int availableWidth = m_scrollArea->viewport()->width();
    if (availableWidth <= 0)
        return;

    const int cardWidth = 220;
    const int spacing = 24;
    const int margins = m_movieGrid->contentsMargins().left()
                        + m_movieGrid->contentsMargins().right();

    int effectiveWidth = availableWidth - margins;
    int newColumns = qMax(3, effectiveWidth / (cardWidth + spacing));
    newColumns = qMin(newColumns, 6);

    if (newColumns != m_gridColumns) {
        m_gridColumns = newColumns;

        QList<QWidget *> widgets;
        for (auto *card : m_movieCards) {
            widgets.append(card);
        }

        QLayoutItem *child;
        while ((child = m_movieGrid->takeAt(0)) != nullptr) {
            delete child;
        }

        int row = 0, col = 0;
        for (QWidget *widget : widgets) {
            m_movieGrid->addWidget(widget, row, col);
            col++;
            if (col >= m_gridColumns) {
                col = 0;
                row++;
            }
        }
        m_scrollContent->adjustSize();
    }
}
