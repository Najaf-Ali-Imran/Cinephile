#include "RecommendationPageWidget.h"
#include "UserManager.h"
#include "MovieCard.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QResizeEvent>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QSet>
#include <algorithm>
#include <QUrlQuery>

RecommendationPageWidget::RecommendationPageWidget(QNetworkAccessManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
    , m_pendingDetailsRequests(0)
{
    setupUi();
    setupChatbotButton();
}

RecommendationPageWidget::~RecommendationPageWidget() {}

void RecommendationPageWidget::setupUi()
{
    this->setObjectName("recommendationPage");
    this->setStyleSheet(QString("QWidget#recommendationPage { background-color: %1; }").arg(AppTheme::MAIN_WINDOW_BG));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 30, 40, 30);
    m_mainLayout->setSpacing(20);

    QWidget* headerContainer = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerContainer);
    headerLayout->setContentsMargins(0,0,0,0);
    headerLayout->setSpacing(15);

    QVBoxLayout* titleVLayout = new QVBoxLayout();
    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK));

    m_subtitleLabel = new QLabel();
    m_subtitleLabel->setStyleSheet(QString("color: %1; font-size: 16px;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    titleVLayout->addWidget(m_titleLabel);
    titleVLayout->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleVLayout);
    m_mainLayout->addWidget(headerContainer);
    qDebug() << "[DEBUG] setupUi: Header created.";

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollContent = new QWidget(m_scrollArea);
    m_scrollContent->setStyleSheet("background: transparent;");

    m_resultsGrid = new QGridLayout(m_scrollContent);
    m_resultsGrid->setContentsMargins(10, 10, 10, 10);
    m_resultsGrid->setSpacing(20);
    m_resultsGrid->setAlignment(Qt::AlignTop);

    m_statusLabel = new QLabel(m_scrollContent);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    m_resultsGrid->addWidget(m_statusLabel, 0, 0, 1, 5);

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea, 1);
    qDebug() << "[DEBUG] setupUi: Scroll area and grid created.";
}


void RecommendationPageWidget::setupChatbotButton()
{
    m_chatbotButton = new QPushButton(this);
    m_chatbotButton->setObjectName("chatbotButton");
    m_chatbotButton->setFixedSize(64, 64);
    m_chatbotButton->setIcon(QIcon(":/assets/icons/chatbot.svg"));
    m_chatbotButton->setIconSize(QSize(32, 32));
    m_chatbotButton->setCursor(Qt::PointingHandCursor);
    m_chatbotButton->setStyleSheet(
        "QPushButton#chatbotButton {"
        "   background-color: #00000;"
        "   border: none;"
        "   border-radius: 32px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 5);
    m_chatbotButton->setGraphicsEffect(shadow);

    m_chatbotPanel = new QWidget(this);
    m_chatbotPanel->setObjectName("chatbotPanel");
    m_chatbotPanel->setFixedWidth(350);
    m_chatbotPanel->setStyleSheet("background-color: #000000; border: none;");
    m_chatbotPanel->hide();

    QVBoxLayout* panelLayout = new QVBoxLayout(m_chatbotPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    m_chatbot = new ChatbotWidget(m_chatbotPanel);
    panelLayout->addWidget(m_chatbot);

    connect(m_chatbot, &ChatbotWidget::closeRequested, this, &RecommendationPageWidget::toggleChatbotPanel);

    connect(m_chatbotButton, &QPushButton::clicked, this, &RecommendationPageWidget::toggleChatbotPanel);
}


void RecommendationPageWidget::generateRecommendations()
{
    clearGrid();
    updateHeader("Recommended For You", "Based on your library activity.");

    if (!UserManager::instance()->isAuthenticated()) {
        showStatusMessage("Please log in to use the recommendation feature.");
        return;
    }

    QJsonObject profile = UserManager::instance()->getProfileData();
    QJsonArray favorites = profile["favorites"].toArray();
    QJsonArray history = profile["watchedHistory"].toArray();

    QList<int> movieIds;
    for (const auto& val : favorites) movieIds.append(val.toInt());
    for (const auto& val : history) movieIds.append(val.toInt());

    QSet<int> uniqueIds;
    for (int id : movieIds) {
        uniqueIds.insert(id);
    }

    if (uniqueIds.isEmpty()) {
        showStatusMessage("Watch and favorite some movies to get personalized recommendations!");
        return;
    }

    analyzeTasteProfile(uniqueIds);
}

void RecommendationPageWidget::analyzeTasteProfile(const QSet<int>& movieIds)
{
    showStatusMessage("Analyzing your taste profile...");
    m_genreScores.clear();
    m_keywordScores.clear();
    m_pendingDetailsRequests = movieIds.size();

    if (m_pendingDetailsRequests == 0) {
        discoverMoviesFromTaste();
        return;
    }

    for (int id : movieIds) {
        fetchTasteProfileDetails(id);
    }
}

void RecommendationPageWidget::fetchTasteProfileDetails(int movieId)
{
    QUrl url(QString("https://api.themoviedb.org/3/movie/%1?api_key=%2&append_to_response=keywords").arg(movieId).arg(m_apiKey));
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processTasteProfileReply(reply);
        reply->deleteLater();
    });
}

void RecommendationPageWidget::processTasteProfileReply(QNetworkReply *reply)
{
    m_pendingDetailsRequests--;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject movieData = QJsonDocument::fromJson(reply->readAll()).object();

        for (const auto& genreVal : movieData["genres"].toArray()) {
            m_genreScores[genreVal.toObject()["id"].toInt()]++;
        }
        for (const auto& keywordVal : movieData["keywords"].toObject()["keywords"].toArray()) {
            m_keywordScores[keywordVal.toObject()["id"].toInt()]++;
        }
    }

    if (m_pendingDetailsRequests <= 0) {
        discoverMoviesFromTaste();
    }
}

void RecommendationPageWidget::discoverMoviesFromTaste()
{
    if (m_genreScores.isEmpty() && m_keywordScores.isEmpty()) {
        showStatusMessage("Could not build a taste profile. Try adding more movies to your library.");
        return;
    }

    showStatusMessage("Curating a list of movies just for you...");

    auto getTopIds = [](const QMap<int, int>& scores, int count) -> QStringList {
        QList<QPair<int, int>> sortedList;
        for (auto it = scores.constBegin(); it != scores.constEnd(); ++it) {
            sortedList.append(qMakePair(it.key(), it.value()));
        }
        std::sort(sortedList.begin(), sortedList.end(), [](const QPair<int, int>& a, const QPair<int, int>& b) {
            return a.second > b.second;
        });

        QStringList topIds;
        for (int i = 0; i < qMin(count, sortedList.size()); ++i) {
            topIds.append(QString::number(sortedList[i].first));
        }
        return topIds;
    };

    QStringList topGenres = getTopIds(m_genreScores, 3);
    QStringList topKeywords = getTopIds(m_keywordScores, 5);

    QUrl url("https://api.themoviedb.org/3/discover/movie");
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    query.addQueryItem("sort_by", "popularity.desc");
    query.addQueryItem("include_adult", "false");
    query.addQueryItem("vote_count.gte", "100");
    if (!topGenres.isEmpty())
        query.addQueryItem("with_genres", topGenres.join('|'));
    if (!topKeywords.isEmpty())
        query.addQueryItem("with_keywords", topKeywords.join('|'));

    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processFinalRecommendationsReply(reply);
        reply->deleteLater();
    });
}

void RecommendationPageWidget::processFinalRecommendationsReply(QNetworkReply *reply)
{
    clearGrid();

    if (reply->error() != QNetworkReply::NoError) {
        showStatusMessage("Sorry, an error occurred while fetching recommendations.");
        return;
    }

    QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object();
    QJsonArray recommendations = data["results"].toArray();

    if (recommendations.isEmpty()) {
        showStatusMessage("We couldn't find any new movies based on your profile. Try watching something different!");
        return;
    }

    if (!m_resultsGrid || !m_resultsGrid->parentWidget()) {
        qCritical() << "[CRITICAL] Attempting to add widgets to an invalid grid layout! Aborting.";
        return;
    }

    int row = 0, col = 0;
    int maxCols = 5;
    for (const auto& val : recommendations) {
        QJsonObject movieData = val.toObject();
        MovieCard *card = new MovieCard(movieData, "https://image.tmdb.org/t/p/", "w342", m_scrollContent);
        connect(card, &MovieCard::clicked, this, [this](const QJsonObject& itemData){
            emit movieClicked(itemData.value("id").toInt(), "movie");
        });
        m_movieCards.append(card);
        m_resultsGrid->addWidget(card, row, col);
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
}


void RecommendationPageWidget::toggleChatbotPanel()
{
    int startX, endX;
    if (m_chatbotPanel->isHidden()) {
        startX = this->width();
        endX = this->width() - m_chatbotPanel->width();
        m_chatbotPanel->setGeometry(startX, 0, m_chatbotPanel->width(), this->height());
        m_chatbotPanel->show();
    } else {
        startX = m_chatbotPanel->x();
        endX = this->width();
    }

    QPropertyAnimation *anim = new QPropertyAnimation(m_chatbotPanel, "pos");
    anim->setDuration(300);
    anim->setStartValue(QPoint(startX, 0));
    anim->setEndValue(QPoint(endX, 0));
    anim->setEasingCurve(QEasingCurve::InOutQuad);

    if (!m_chatbotPanel->isHidden() && endX > startX) {
        connect(anim, &QPropertyAnimation::finished, m_chatbotPanel, &QWidget::hide, Qt::SingleShotConnection);
    }

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}


void RecommendationPageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_chatbotButton->move(this->width() - m_chatbotButton->width() - 25, this->height() - m_chatbotButton->height() - 25);

    if (!m_chatbotPanel->isHidden()) {
        m_chatbotPanel->setGeometry(this->width() - m_chatbotPanel->width(), 0, m_chatbotPanel->width(), this->height());
    }
}

void RecommendationPageWidget::clearGrid()
{
    m_statusLabel->hide();
    qDeleteAll(m_movieCards);
    m_movieCards.clear();

    QLayoutItem *child;
    while ((child = m_resultsGrid->takeAt(0)) != nullptr) {
        if (child->widget() && child->widget() != m_statusLabel) {
            child->widget()->deleteLater();
        }
        if (child->widget() != m_statusLabel) {
            delete child;
        }
    }
}

void RecommendationPageWidget::showStatusMessage(const QString& message)
{
    clearGrid();
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_statusLabel->show();
}

void RecommendationPageWidget::updateHeader(const QString& title, const QString& subtitle)
{
    m_titleLabel->setText(title);
    m_subtitleLabel->setText(subtitle);
}
