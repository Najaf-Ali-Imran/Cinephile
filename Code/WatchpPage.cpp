#include "MovieCard.h"
#include "NativeVideoPlayer.h"
#include "Theme.h"
#include "WatchPage.h"

#include <QDate>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStackedLayout>
#include <QUrlQuery>
#include <QVBoxLayout>

WatchPage::WatchPage(QNetworkAccessManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
{
    setupUi();
    loadInitialMovies();
}

WatchPage::~WatchPage() {}

void WatchPage::setupUi()
{
    this->setObjectName("watchPage");
    this->setStyleSheet(
        QString("QWidget#watchPage { background-color: %1; }").arg(AppTheme::MAIN_WINDOW_BG));

    m_mainStack = new QStackedLayout(this);
    m_mainStack->setContentsMargins(0, 0, 0, 0);

    m_selectionWidget = new QWidget(this);
    setupSelectionUi(m_selectionWidget);

    // Create the player container
    m_playerContainer = new QWidget(this);
    QVBoxLayout *playerLayout = new QVBoxLayout(m_playerContainer);
    playerLayout->setContentsMargins(10, 10, 10, 10);
    playerLayout->setSpacing(10);

    m_backToSelectionButton = new QPushButton("← Back to Movie Selection");
    m_backToSelectionButton->setCursor(Qt::PointingHandCursor);
    m_backToSelectionButton->setFixedHeight(40);
    m_backToSelectionButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border: none; border-radius: 8px; "
                "padding: 0 20px; font-weight: bold; font-size: 14px; }"
                "QPushButton:hover { background-color: %3; }")
            .arg(AppTheme::BUTTON_SECONDARY_BG,
                 AppTheme::TEXT_ON_DARK,
                 AppTheme::BUTTON_SECONDARY_HOVER_BG));
    connect(m_backToSelectionButton, &QPushButton::clicked, this, &WatchPage::showMovieSelection);

    m_player = new NativeVideoPlayer(m_playerContainer);

    playerLayout->addWidget(m_backToSelectionButton, 0, Qt::AlignLeft);
    playerLayout->addWidget(m_player, 1);

    m_mainStack->addWidget(m_selectionWidget);
    m_mainStack->addWidget(m_playerContainer);

    m_mainStack->setCurrentWidget(m_selectionWidget);
}

void WatchPage::setupSelectionUi(QWidget *container)
{
    container->setObjectName("selectionWidget");
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(25, 20, 25, 20);
    layout->setSpacing(20);

    QLabel *titleLabel = new QLabel("Watch Movies", container);
    titleLabel->setStyleSheet(
        QString("color: %1; font-size: 28px; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK));

    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_searchBar = new QLineEdit(container);
    m_searchBar->setPlaceholderText("Search for a movie to watch...");
    m_searchBar->setFixedHeight(40);
    m_searchBar->setStyleSheet(
        QString("QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: "
                "8px; padding: 5px 15px; font-size: 14px; }"
                "QLineEdit:focus { border: 1px solid %4; }")
            .arg(AppTheme::INPUT_BG,
                 AppTheme::TEXT_ON_DARK,
                 AppTheme::INPUT_BORDER,
                 AppTheme::PRIMARY_RED));
    connect(m_searchBar, &QLineEdit::returnPressed, this, &WatchPage::onSearchSubmitted);

    m_searchButton = new QPushButton("Search", container);
    m_searchButton->setFixedHeight(40);
    m_searchButton->setCursor(Qt::PointingHandCursor);
    m_searchButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border: none; border-radius: 8px; "
                "padding: 0 25px; font-weight: bold; }"
                "QPushButton:hover { background-color: %3; }")
            .arg(AppTheme::PRIMARY_RED, AppTheme::TEXT_ON_DARK, AppTheme::BUTTON_HOVER_DARK_BG));
    connect(m_searchButton, &QPushButton::clicked, this, &WatchPage::onSearchSubmitted);

    searchLayout->addWidget(m_searchBar, 1);
    searchLayout->addWidget(m_searchButton);

    m_scrollArea = new QScrollArea(container);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");

    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet("background: transparent;");
    m_movieGrid = new QGridLayout(m_scrollContent);
    m_movieGrid->setContentsMargins(10, 10, 10, 10);
    m_movieGrid->setSpacing(20);

    m_scrollArea->setWidget(m_scrollContent);

    m_statusLabel = new QLabel(m_scrollContent);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->hide();

    layout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    layout->addLayout(searchLayout);
    layout->addWidget(m_scrollArea, 1);
}

void WatchPage::loadInitialMovies()
{
    if (m_isFetching)
        return;
    m_isFetching = true;

    clearGrid();
    showStatusMessage("Loading recent movies...", false);

    QDate today = QDate::currentDate();
    QDate twoMonthsAgo = today.addMonths(-2);

    QUrl url("https://api.themoviedb.org/3/discover/movie");
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    query.addQueryItem("language", "en-US");
    query.addQueryItem("sort_by", "popularity.desc");
    query.addQueryItem("primary_release_date.gte", twoMonthsAgo.toString(Qt::ISODate));
    query.addQueryItem("primary_release_date.lte", today.toString(Qt::ISODate));
    query.addQueryItem("vote_count.gte", "100");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processMovieReply(reply);
        reply->deleteLater();
    });
}

void WatchPage::onSearchSubmitted()
{
    QString queryText = m_searchBar->text().trimmed();
    if (queryText.isEmpty()) {
        loadInitialMovies();
        return;
    }

    if (m_isFetching)
        return;
    m_isFetching = true;

    clearGrid();
    showStatusMessage(QString("Searching for '%1'...").arg(queryText), false);

    QUrl url("https://api.themoviedb.org/3/search/movie");
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    query.addQueryItem("language", "en-US");
    query.addQueryItem("query", queryText);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processMovieReply(reply);
        reply->deleteLater();
    });
}

void WatchPage::processMovieReply(QNetworkReply *reply)
{
    m_isFetching = false;

    if (reply->error()) {
        showStatusMessage("Error: " + reply->errorString(), true);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray results = doc.object()["results"].toArray();

    if (results.isEmpty()) {
        showStatusMessage("No movies found.", false);
        return;
    }

    m_statusLabel->hide();

    int row = 0, col = 0;
    for (const auto &val : results) {
        QJsonObject contentObj = val.toObject();
        if (contentObj["poster_path"].isNull())
            continue;

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
    m_scrollContent->adjustSize();
}

void WatchPage::onMovieCardClicked(const QJsonObject &itemData)
{
    int tmdbId = itemData["id"].toInt();
    QString backdropPath = itemData["backdrop_path"].toString();
    qDebug() << "Movie card clicked. TMDB ID:" << tmdbId;
    playMovie(tmdbId, backdropPath);
}

void WatchPage::playMovie(int tmdbId, const QString &backdropPath)
{
    m_mainStack->setCurrentWidget(m_playerContainer);
    m_player->playMovie(tmdbId, backdropPath);
}

void WatchPage::showMovieSelection()
{
    if (m_player) {
        m_player->stop();
    }
    m_mainStack->setCurrentWidget(m_selectionWidget);
}

void WatchPage::clearGrid()
{
    m_statusLabel->hide();
    qDeleteAll(m_movieCards);
    m_movieCards.clear();

    QLayoutItem *child;
    while ((child = m_movieGrid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void WatchPage::showStatusMessage(const QString &message, const QString &color)
{
    clearGrid();
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(
        QString("color: %1; font-size: 16px; font-weight: bold;").arg(color));
    m_statusLabel->show();
    m_movieGrid->addWidget(m_statusLabel, 0, 0, 1, m_gridColumns, Qt::AlignCenter);
}

void WatchPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_scrollContent || !m_movieGrid)
        return;

    int cardWidthEstimate = 200 + m_movieGrid->spacing();
    if (m_scrollContent->width() <= 0)
        return;

    int newColumns = qMax(2, m_scrollContent->width() / cardWidthEstimate);

    if (newColumns != m_gridColumns) {
        m_gridColumns = newColumns;
        relayoutGrid();
    }
}

void WatchPage::relayoutGrid()
{
    int row = 0, col = 0;
    for (QWidget *widget : m_movieCards) {
        if (widget) {
            m_movieGrid->addWidget(widget, row, col);
            col++;
            if (col >= m_gridColumns) {
                col = 0;
                row++;
            }
        }
    }
}
