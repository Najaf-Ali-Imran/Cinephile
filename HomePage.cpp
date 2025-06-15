#include "HomePage.h"
#include <QDebug>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedLayout>
#include <QUrlQuery>
#include "MovieCard.h"
#include "Theme.h"

const QList<TmdbEndpoint> ALL_ENDPOINTS
    = {{"/3/trending/movie/week", "Featured Highlight", "hero"},
       {"/3/movie/now_playing", "Now Playing Movies", "movie"},
       {"/3/trending/movie/week", "Trending Movies This Week", "movie"},
       {"/3/movie/popular", "Popular Movies", "movie"},
       {"/3/movie/top_rated", "Top Rated Movies", "movie"},
       {"/3/movie/upcoming", "Upcoming Movies", "movie"},
       {"/3/tv/on_the_air", "TV Shows On The Air", "tv"},
       {"/3/trending/tv/week", "Trending TV Shows This Week", "tv"},
       {"/3/tv/popular", "Popular TV Shows", "tv"},
       {"/3/tv/top_rated", "Top Rated TV Shows", "tv"}};

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_imageConfigManager(new QNetworkAccessManager(this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_scrollContentWidget(new QWidget())
    , m_scrollContentLayout(new QVBoxLayout(m_scrollContentWidget))
    , m_tmdbPosterSize("w780")
    , m_tmdbBackdropSize("w1280")
{
    m_endpoints = ALL_ENDPOINTS;
    setupUI();
    connect(m_imageConfigManager,
            &QNetworkAccessManager::finished,
            this,
            &HomePage::handleImageConfigResponse);
    connect(m_networkManager,
            &QNetworkAccessManager::finished,
            this,
            &HomePage::handleNetworkResponse);

    fetchTmdbConfiguration();
}

HomePage::~HomePage()
{
    clearLayout(m_scrollContentLayout);
}

void HomePage::setupUI()
{
    this->setStyleSheet(QString("background-color: %1;").arg(AppTheme::MAIN_WINDOW_BG));
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent; border: none;");
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_scrollContentWidget->setStyleSheet("background: transparent;");
    m_scrollContentLayout->setContentsMargins(0, 0, 0, 40);
    m_scrollContentLayout->setSpacing(50);

    scrollArea->setWidget(m_scrollContentWidget);
    m_mainLayout->addWidget(scrollArea);
}

void HomePage::fetchTmdbConfiguration()
{
    if (m_loading && m_pendingRequests > 0) {
        return;
    }
    m_loading = true;
    m_pendingRequests++;

    QUrl url("https://api.themoviedb.org/3/configuration");
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::User, "config_request");
    m_imageConfigManager->get(request);
}

void HomePage::handleImageConfigResponse(QNetworkReply *reply)
{
    m_pendingRequests--;
    if (reply->request().attribute(QNetworkRequest::User).toString() != "config_request") {
        reply->deleteLater();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit loadFailed("Failed to load TMDb configuration: " + reply->errorString());
        if (m_pendingRequests == 0)
            m_loading = false;
    } else {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject config = doc.object();
        QJsonObject imagesConfig = config["images"].toObject();
        m_tmdbImageBaseUrl = imagesConfig["secure_base_url"].toString();

        QStringList preferredPosterSizes = {"w780", "w500", "original"};
        QStringList preferredBackdropSizes = {"w1280", "w780", "original"};

        m_tmdbPosterSize = preferredPosterSizes.first();
        m_tmdbBackdropSize = preferredBackdropSizes.first();

        loadData();
    }
    reply->deleteLater();
    if (m_pendingRequests == 0) {
        m_loading = false;
    }
}

void HomePage::loadData()
{
    if (m_tmdbImageBaseUrl.isEmpty()) {
        if (!m_loading)
            fetchTmdbConfiguration();
        return;
    }

    clearLayout(m_scrollContentLayout);
    m_sectionsMap.clear();

    m_loading = true;
    bool hasEndpointsToLoad = false;

    for (const auto &endpoint : m_endpoints) {
        m_pendingRequests++;
        hasEndpointsToLoad = true;

        QUrl url("https://api.themoviedb.org" + endpoint.path);
        QUrlQuery query;
        query.addQueryItem("api_key", m_apiKey);
        query.addQueryItem("language", "en-US");
        query.addQueryItem("page", "1");
        url.setQuery(query);

        QNetworkRequest request{url};
        request.setRawHeader("Accept", "application/json");
        request.setAttribute(QNetworkRequest::User,
                             QVariant::fromValue(endpoint.path + "||" + endpoint.title + "||"
                                                 + endpoint.type));
        m_networkManager->get(request);

        if (endpoint.type != "hero") {
            QWidget *sectionContainer = createSection(endpoint.title, endpoint.type);
            m_sectionsMap.insert(endpoint.path + "||" + endpoint.title, sectionContainer);
        }
    }

    if (!hasEndpointsToLoad) {
        if (m_pendingRequests == 0)
            m_loading = false;
        emit allDataLoaded();
    }
}

void HomePage::handleNetworkResponse(QNetworkReply *reply)
{
    m_pendingRequests--;

    QString requestUserData = reply->request().attribute(QNetworkRequest::User).toString();
    QStringList userDataParts = requestUserData.split("||");
    if (userDataParts.count() < 3) {
        reply->deleteLater();
        return;
    }
    QString endpointPath = userDataParts.value(0);
    QString sectionTitle = userDataParts.value(1);
    QString sectionType = userDataParts.value(2);

    if (reply->error() != QNetworkReply::NoError) {
        emit loadFailed("Error for " + sectionTitle + ": " + reply->errorString());
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject P_jsonObject = doc.object();
        QJsonArray items = P_jsonObject["results"].toArray();

        if (!items.isEmpty()) {
            if (sectionType == "hero") {
                createHeroBanner(items.first().toObject());
            } else {
                QWidget *sectionWidget = m_sectionsMap.value(endpointPath + "||" + sectionTitle);
                if (sectionWidget) {
                    populateSection(sectionWidget, items, sectionType);
                }
            }
        }
        emit dataLoaded();
    }

    reply->deleteLater();

    if (m_pendingRequests == 0) {
        m_loading = false;
        emit allDataLoaded();
    }
}

void HomePage::createHeroBanner(const QJsonObject &itemData)
{
    if (m_tmdbImageBaseUrl.isEmpty() || itemData.isEmpty())
        return;

    QWidget *heroContainer = new QWidget(m_scrollContentWidget);
    heroContainer->setMinimumHeight(550);
    heroContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    heroContainer->setObjectName("HeroBannerContainer");

    QStackedLayout *stackedLayout = new QStackedLayout(heroContainer);
    stackedLayout->setContentsMargins(0, 0, 0, 0);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);

    QLabel *blurredBackdropLabel = new QLabel();
    blurredBackdropLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    blurredBackdropLabel->setAlignment(Qt::AlignCenter);
    blurredBackdropLabel->setScaledContents(true);
    auto *blurEffect = new QGraphicsBlurEffect();
    blurEffect->setBlurRadius(0);
    blurredBackdropLabel->setGraphicsEffect(blurEffect);

    QLabel *backdropLabel = new QLabel();
    backdropLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    backdropLabel->setAlignment(Qt::AlignCenter);
    backdropLabel->setScaledContents(true);
    backdropLabel->setStyleSheet("background-color: #1a1a1a;");

    auto *fadeEffect = new QGraphicsOpacityEffect();
    fadeEffect->setOpacity(0.0);
    backdropLabel->setGraphicsEffect(fadeEffect);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(fadeEffect, "opacity");
    fadeIn->setDuration(1000);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    QLabel *gradientOverlay = new QLabel();
    gradientOverlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gradientOverlay->setStyleSheet(R"(
        background: qlineargradient(
            x1: 0, y1: 0, x2: 0, y2: 1,
            stop: 0 rgba(0,0,0,0),
            stop: 0.4 rgba(0,0,0,0.4),
            stop: 1 rgba(0,0,0,0.85)
        );
    )");

    QWidget *contentOverlay = new QWidget();
    contentOverlay->setStyleSheet("background-color: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentOverlay);
    contentLayout->setContentsMargins(50, 50, 50, 50);

    QString heroTitleText = itemData.contains("title") ? itemData["title"].toString()
                                                       : itemData["name"].toString();
    QLabel *titleLabel = new QLabel(heroTitleText);
    titleLabel->setStyleSheet("color: white; font-size: 48px; font-weight: bold;");
    titleLabel->setWordWrap(true);
    titleLabel->setMaximumWidth(600);

    QLabel *overviewLabel = new QLabel(itemData["overview"].toString());
    overviewLabel->setStyleSheet("color: #DDDDDD; font-size: 16px; line-height: 1.5;");
    overviewLabel->setWordWrap(true);
    overviewLabel->setMaximumWidth(550);

    contentLayout->addStretch();
    contentLayout->addWidget(titleLabel);
    contentLayout->addSpacing(20);
    contentLayout->addWidget(overviewLabel);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(15);
    actionsLayout->setContentsMargins(0, 25, 0, 0);

    QPushButton *watchNowButton = new QPushButton("Watch Now");
    watchNowButton->setCursor(Qt::PointingHandCursor);
    watchNowButton->setMinimumHeight(45);
    watchNowButton->setStyleSheet(
        "QPushButton { background-color: #E50914; color: white; border: none; "
        "border-radius: 8px; padding: 0 30px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #F40612; }");
    connect(watchNowButton, &QPushButton::clicked, this, [this, itemData]() {
        onMovieCardClicked(itemData);
    });

    actionsLayout->addWidget(watchNowButton);
    actionsLayout->addStretch();

    contentLayout->addLayout(actionsLayout);
    contentLayout->addStretch();

    stackedLayout->addWidget(blurredBackdropLabel);
    stackedLayout->addWidget(backdropLabel);
    stackedLayout->addWidget(gradientOverlay);
    stackedLayout->addWidget(contentOverlay);

    contentOverlay->raise();

    if (m_scrollContentLayout->count() > 0)
        m_scrollContentLayout->insertWidget(0, heroContainer);
    else
        m_scrollContentLayout->addWidget(heroContainer);

    QString backdropPath = itemData["backdrop_path"].toString();
    if (backdropPath.isEmpty()) {
        tryFallbackImage(itemData, backdropLabel);
    } else {
        QUrl backdropUrl(m_tmdbImageBaseUrl + m_tmdbBackdropSize + backdropPath);
        auto *imageDownloader = new QNetworkAccessManager(this);
        connect(imageDownloader,
                &QNetworkAccessManager::finished,
                [=](QNetworkReply *imgReply) {
                    if (imgReply->error() == QNetworkReply::NoError) {
                        QPixmap pixmap;
                        if (pixmap.loadFromData(imgReply->readAll())) {
                            QPixmap scaled = pixmap.scaled(
                                backdropLabel->size(),
                                Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);
                            backdropLabel->setPixmap(scaled);
                            blurredBackdropLabel->setPixmap(scaled);
                            fadeIn->start();
                        } else {
                            tryFallbackImage(itemData, backdropLabel);
                        }
                    } else {
                        tryFallbackImage(itemData, backdropLabel);
                    }
                    imgReply->deleteLater();
                    imageDownloader->deleteLater();
                });
        imageDownloader->get(QNetworkRequest{backdropUrl});
    }
}

void HomePage::tryFallbackImage(const QJsonObject &itemData, QLabel *imageLabel)
{
    QString posterPath = itemData["poster_path"].toString();
    if (!posterPath.isEmpty()) {
        QUrl posterUrl(m_tmdbImageBaseUrl + m_tmdbPosterSize + posterPath);
        auto *fallbackDownloader = new QNetworkAccessManager(this);
        connect(fallbackDownloader,
                &QNetworkAccessManager::finished,
                [imageLabel, fallbackDownloader](QNetworkReply *reply) {
                    if (reply->error() == QNetworkReply::NoError) {
                        QPixmap pixmap;
                        if (pixmap.loadFromData(reply->readAll())) {
                            imageLabel->setPixmap(pixmap);
                        }
                    }
                    reply->deleteLater();
                    fallbackDownloader->deleteLater();
                });
        fallbackDownloader->get(QNetworkRequest{posterUrl});
    } else {
        imageLabel->setStyleSheet("background-color: #1a1a1a;");
    }
}

QWidget *HomePage::createHorizontalScrollerWithArrows(QHBoxLayout *contentLayout)
{
    QWidget *container = new QWidget();
    container->setMinimumHeight(420);
    container->installEventFilter(this);

    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent; border: none;");

    QWidget *contentWidget = new QWidget();
    contentWidget->setLayout(contentLayout);
    scrollArea->setWidget(contentWidget);

    mainLayout->addWidget(scrollArea);

    auto createArrowButton = [&](const QString &iconPath) {
        QPushButton *btn = new QPushButton(container);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(24, 24));
        btn->setFixedSize(50, 80);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("scrollArrowButton");
        btn->setStyleSheet("QPushButton#scrollArrowButton {"
                           "   background-color: rgba(20, 20, 20, 0.6);"
                           "   border-radius: 10px;"
                           "   border: none;"
                           "}"
                           "QPushButton#scrollArrowButton:hover {"
                           "   background-color: rgba(0, 0, 0, 0.8);"
                           "}");
        btn->setVisible(false);
        btn->raise();
        return btn;
    };

    QPushButton *leftButton = createArrowButton(":assets/icons/arrow_left.svg");
    QPushButton *rightButton = createArrowButton(":assets/icons/arrow_right.svg");

    container->setProperty("leftButton", QVariant::fromValue<QObject *>(leftButton));
    container->setProperty("rightButton", QVariant::fromValue<QObject *>(rightButton));
    container->setProperty("scrollArea", QVariant::fromValue<QObject *>(scrollArea));

    auto scrollLambda = [scrollArea](int offset) {
        QPropertyAnimation *anim = new QPropertyAnimation(scrollArea->horizontalScrollBar(), "value");
        anim->setDuration(400);
        anim->setStartValue(scrollArea->horizontalScrollBar()->value());
        anim->setEndValue(scrollArea->horizontalScrollBar()->value() + offset);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };

    connect(leftButton, &QPushButton::clicked, this, [scrollLambda, scrollArea]() {
        scrollLambda(-scrollArea->viewport()->width() * 0.9);
    });
    connect(rightButton, &QPushButton::clicked, this, [scrollLambda, scrollArea]() {
        scrollLambda(scrollArea->viewport()->width() * 0.9);
    });

    auto visibilityUpdater = [container, leftButton, rightButton, scrollArea]() {
        bool show = container->underMouse();
        leftButton->setVisible(show && scrollArea->horizontalScrollBar()->value() > 0);
        rightButton->setVisible(show && scrollArea->horizontalScrollBar()->value() < scrollArea->horizontalScrollBar()->maximum());
    };

    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, visibilityUpdater);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, visibilityUpdater);

    return container;
}

bool HomePage::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *container = qobject_cast<QWidget *>(watched);
    if (container && container->property("scrollArea").isValid()) {
        auto leftButton = qobject_cast<QPushButton *>(container->property("leftButton").value<QObject *>());
        auto rightButton = qobject_cast<QPushButton *>(container->property("rightButton").value<QObject *>());
        auto scrollArea = qobject_cast<QScrollArea *>(container->property("scrollArea").value<QObject *>());

        if (leftButton && rightButton && scrollArea) {
            auto updateVisibility = [&]() {
                bool show = container->underMouse();
                leftButton->setVisible(show && scrollArea->horizontalScrollBar()->value() > 0);
                rightButton->setVisible(show && scrollArea->horizontalScrollBar()->value() < scrollArea->horizontalScrollBar()->maximum());
            };

            if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
                leftButton->move(10, (container->height() - leftButton->height()) / 2);
                rightButton->move(container->width() - rightButton->width() - 10, (container->height() - rightButton->height()) / 2);
            }

            switch (event->type()) {
            case QEvent::Enter:
            case QEvent::Leave:
                updateVisibility();
                return false;
            default:
                break;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}


QWidget *HomePage::createSection(const QString &title, const QString &sectionType)
{
    QWidget *sectionContainer = new QWidget(m_scrollContentWidget);
    sectionContainer->setObjectName("SectionContainer_" + title);

    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionContainer);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(15);

    QLabel *sectionLabel = new QLabel(title, sectionContainer);
    sectionLabel->setStyleSheet(
        QString("color: %1; font-size: 24px; font-weight: 600; padding-left: 40px;")
            .arg(AppTheme::TEXT_ON_DARK));

    sectionLayout->addWidget(sectionLabel);

    QHBoxLayout *itemLayout = new QHBoxLayout();
    itemLayout->setContentsMargins(40, 5, 40, 5);
    itemLayout->setSpacing(24);

    QWidget *scroller = createHorizontalScrollerWithArrows(itemLayout);
    scroller->setProperty("itemLayout", QVariant::fromValue<QObject *>(itemLayout));

    sectionContainer->setProperty("scroller", QVariant::fromValue<QObject *>(scroller));

    sectionLayout->addWidget(scroller);
    m_scrollContentLayout->addWidget(sectionContainer);
    return sectionContainer;
}

void HomePage::populateSection(QWidget *sectionWidget, const QJsonArray &items, const QString &itemType)
{
    if (!sectionWidget)
        return;

    QWidget *scroller = qobject_cast<QWidget *>(sectionWidget->property("scroller").value<QObject *>());
    if (!scroller)
        return;

    QHBoxLayout *itemLayout = qobject_cast<QHBoxLayout *>(scroller->property("itemLayout").value<QObject *>());
    if (!itemLayout)
        return;

    QLayoutItem *child;
    while ((child = itemLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    int maxItemsToShow = 15;
    for (int i = 0; i < qMin(maxItemsToShow, items.size()); ++i) {
        QJsonObject itemData = items[i].toObject();
        if (itemData["poster_path"].isNull())
            continue;
        MovieCard *card = new MovieCard(itemData, m_tmdbImageBaseUrl, m_tmdbPosterSize, scroller);
        connect(card, &MovieCard::clicked, this, &HomePage::onMovieCardClicked);
        itemLayout->addWidget(card);
    }
    itemLayout->addStretch();
}

void HomePage::onMovieCardClicked(const QJsonObject &itemData)
{
    QString type = itemData.contains("title") ? "movie" : "tv";
    int id = itemData["id"].toInt();
    emit movieClicked(id, type);
}

void HomePage::clearLayout(QLayout *layout)
{
    if (!layout)
        return;
    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
            widget->deleteLater();

        if (QLayout *childLayout = item->layout())
            clearLayout(childLayout);

        delete item;
    }
}
