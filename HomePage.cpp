#include "HomePage.h"
#include "MovieCard.h" // Ensure this is the updated MovieCard.h
#include <QScrollArea>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout> // For Hero Banner

// Define your TMDb endpoints
const QList<TmdbEndpoint> ALL_ENDPOINTS = {
    // {"/3/configuration", "Configuration", "config"}, // Handled separately
    {"/3/trending/movie/week", "Featured Highlight", "hero"},
    {"/3/movie/now_playing", "Now Playing Movies", "movie"},
    {"/3/trending/movie/week", "Trending Movies This Week", "movie"},
    {"/3/movie/popular", "Popular Movies", "movie"},
    {"/3/movie/top_rated", "Top Rated Movies", "movie"},
    {"/3/movie/upcoming", "Upcoming Movies", "movie"},
    {"/3/tv/on_the_air", "TV Shows On The Air", "tv"},
    {"/3/trending/tv/week", "Trending TV Shows This Week", "tv"},
    {"/3/tv/popular", "Popular TV Shows", "tv"},
    {"/3/tv/top_rated", "Top Rated TV Shows", "tv"},
    {"/3/tv/airing_today", "TV Shows Airing Today", "tv"}
};

HomePage::HomePage(QWidget *parent)
    : QWidget(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_imageConfigManager(new QNetworkAccessManager(this)),
    m_mainLayout(new QVBoxLayout(this)),
    m_scrollContentWidget(new QWidget()),
    m_scrollContentLayout(new QVBoxLayout(m_scrollContentWidget)),
    m_tmdbPosterSize("w342"), // Default poster size
    m_tmdbBackdropSize("w780") // Default backdrop size
{
    m_endpoints = ALL_ENDPOINTS;
    setupUI();
    connect(m_imageConfigManager, &QNetworkAccessManager::finished, this, &HomePage::handleImageConfigResponse);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &HomePage::handleNetworkResponse);

    fetchTmdbConfiguration();
}

HomePage::~HomePage()
{
    clearLayout(m_scrollContentLayout);
    // m_networkManager and m_imageConfigManager are auto-deleted due to parent=this
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

    m_scrollContentWidget->setStyleSheet("background: transparent;");
    m_scrollContentLayout->setContentsMargins(20, 15, 20, 15);
    m_scrollContentLayout->setSpacing(35); // Space between vertical sections

    scrollArea->setWidget(m_scrollContentWidget);
    m_mainLayout->addWidget(scrollArea);
}

void HomePage::fetchTmdbConfiguration() {
    if (m_loading && m_pendingRequests > 0) { // Already fetching or has pending config request
        qDebug() << "TMDb configuration fetch already in progress.";
        return;
    }
    m_loading = true;
    m_pendingRequests++;

    QUrl url("https://api.themoviedb.org/3/configuration");
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::User, "config_request");
    m_imageConfigManager->get(request);
    qDebug() << "Fetching TMDb configuration...";
}

void HomePage::handleImageConfigResponse(QNetworkReply *reply) {
    m_pendingRequests--;
    if (reply->request().attribute(QNetworkRequest::User).toString() != "config_request") {
        reply->deleteLater(); // Not our config reply
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error loading TMDb configuration:" << reply->errorString();
        emit loadFailed("Failed to load TMDb configuration: " + reply->errorString());
        if (m_pendingRequests == 0) m_loading = false;
    } else {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject config = doc.object();
        QJsonObject imagesConfig = config["images"].toObject();
        m_tmdbImageBaseUrl = imagesConfig["secure_base_url"].toString();

        QStringList preferredPosterSizes = {"w342", "w500", "w185", "w780", "original"};
        QStringList preferredBackdropSizes = {"w780", "w1280", "w300", "original"};

        QJsonArray apiPosterSizes = imagesConfig["poster_sizes"].toArray();
        for(const QString& size : preferredPosterSizes){
            if(apiPosterSizes.contains(QJsonValue(size))){
                m_tmdbPosterSize = size;
                break;
            }
        }
        if (m_tmdbPosterSize.isEmpty() && !apiPosterSizes.isEmpty()) m_tmdbPosterSize = apiPosterSizes.first().toString();


        QJsonArray apiBackdropSizes = imagesConfig["backdrop_sizes"].toArray();
        for(const QString& size : preferredBackdropSizes){
            if(apiBackdropSizes.contains(QJsonValue(size))){
                m_tmdbBackdropSize = size;
                break;
            }
        }
        if (m_tmdbBackdropSize.isEmpty() && !apiBackdropSizes.isEmpty()) m_tmdbBackdropSize = apiBackdropSizes.first().toString();

        qDebug() << "TMDb Config Loaded: Base URL:" << m_tmdbImageBaseUrl << "Poster:" << m_tmdbPosterSize << "Backdrop:" << m_tmdbBackdropSize;
        loadData(); // Proceed to load content
    }
    reply->deleteLater();
    if (m_pendingRequests == 0 && !m_tmdbImageBaseUrl.isEmpty()) {
        // If all requests are done (e.g. only config failed but others were not initiated)
        // and config was successful, loading state should be managed by loadData()
    } else if (m_pendingRequests == 0) {
        m_loading = false; // Reset loading if config failed and no other requests pending
    }
}


void HomePage::loadData()
{
    if (m_tmdbImageBaseUrl.isEmpty()) {
        qDebug() << "TMDb image configuration not loaded yet. Aborting data load. Will retry config.";
        // Attempt to fetch config again if it's missing.
        // This prevents getting stuck if the initial config call failed silently or was missed.
        if (!m_loading) fetchTmdbConfiguration();
        return;
    }

    // Clear existing dynamic content (sections, hero) before loading new
    clearLayout(m_scrollContentLayout);
    m_sectionsMap.clear();


    m_loading = true; // Set loading true as we are starting data fetch
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

        QNetworkRequest request(url);
        request.setRawHeader("Accept", "application/json");
        request.setAttribute(QNetworkRequest::User, QVariant::fromValue(endpoint.path + "||" + endpoint.title + "||" + endpoint.type));
        m_networkManager->get(request);
        qDebug() << "Requesting data for:" << endpoint.title;

        if (endpoint.type != "hero") {
            // Create section structure immediately, populate later
            QWidget* sectionContainer = createSection(endpoint.title, endpoint.type);
            m_sectionsMap.insert(endpoint.path + "||" + endpoint.title, sectionContainer); // Use a more unique key
        }
    }

    if (!hasEndpointsToLoad) { // Should not happen with current m_endpoints
        if (m_pendingRequests == 0) m_loading = false;
        emit allDataLoaded();
    }
}

void HomePage::handleNetworkResponse(QNetworkReply *reply)
{
    m_pendingRequests--;

    QString requestUserData = reply->request().attribute(QNetworkRequest::User).toString();
    QStringList userDataParts = requestUserData.split("||");
    QString endpointPath = userDataParts.value(0);
    QString sectionTitle = userDataParts.value(1);
    QString sectionType = userDataParts.value(2);

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error loading data for" << sectionTitle << ":" << reply->errorString();
        emit loadFailed("Error for " + sectionTitle + ": " + reply->errorString());
    } else {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject P_jsonObject = doc.object();
        QJsonArray items = P_jsonObject["results"].toArray();

        if (items.isEmpty()) {
            qDebug() << "No items returned for" << sectionTitle;
            // Optionally remove the empty section from UI or display a message
            QWidget* sectionWidget = m_sectionsMap.value(endpointPath + "||" + sectionTitle);
            if(sectionWidget && sectionType != "hero"){
                // sectionWidget->setVisible(false); // Hide or delete
            }
        } else {
            if (sectionType == "hero") {
                createHeroBanner(items.first().toObject());
            } else {
                QWidget* sectionWidget = m_sectionsMap.value(endpointPath + "||" + sectionTitle);
                if(sectionWidget){
                    populateSection(sectionWidget, items, sectionType);
                } else {
                    qDebug() << "Could not find section widget for key:" << (endpointPath + "||" + sectionTitle);
                }
            }
        }
        emit dataLoaded();
    }

    reply->deleteLater();

    if (m_pendingRequests == 0) {
        m_loading = false;
        emit allDataLoaded();
        qDebug() << "All data loading finished. Pending requests:" << m_pendingRequests;
    }
}


void HomePage::createHeroBanner(const QJsonObject &itemData) {
    if (m_tmdbImageBaseUrl.isEmpty()) {
        qDebug() << "Image base URL not set, cannot create hero banner.";
        return;
    }

    QWidget *heroContainer = new QWidget(m_scrollContentWidget);
    heroContainer->setMinimumHeight(350); // Increased height for better visual
    heroContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    heroContainer->setObjectName("HeroBannerContainer");

    QVBoxLayout *heroOuterLayout = new QVBoxLayout(heroContainer);
    heroOuterLayout->setContentsMargins(0,0,0,0);

    QStackedLayout *stackedHeroContentLayout = new QStackedLayout();
    stackedHeroContentLayout->setStackingMode(QStackedLayout::StackAll);

    // Backdrop Image Label
    QLabel *backdropLabel = new QLabel(); // No parent initially, will be added to layout
    backdropLabel->setScaledContents(false); // We will scale pixmap manually for better quality
    backdropLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    backdropLabel->setStyleSheet("border-radius: 12px; background-color: #222;"); // Rounded corners for backdrop

    QString backdropPath = itemData["backdrop_path"].toString();
    if (!backdropPath.isEmpty() && !m_tmdbImageBaseUrl.isEmpty()) {
        QUrl backdropUrl(m_tmdbImageBaseUrl + m_tmdbBackdropSize + backdropPath);
        QNetworkAccessManager* imageDownloader = new QNetworkAccessManager(); // Temporary for this hero image
        connect(imageDownloader, &QNetworkAccessManager::finished, [backdropLabel, imageDownloader, this](QNetworkReply* imgReply){
            if (imgReply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(imgReply->readAll());
                if (!pixmap.isNull()) {
                    // Scale pixmap to fill label, keeping aspect ratio and cropping
                    QPixmap scaledPixmap = pixmap.scaled(backdropLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    backdropLabel->setPixmap(scaledPixmap);
                } else {
                    backdropLabel->setText("Image load failed");
                }
            } else {
                qDebug() << "Failed to download hero image:" << imgReply->errorString();
                backdropLabel->setText("Image unavailable");
            }
            imgReply->deleteLater();
            imageDownloader->deleteLater();
        });
        imageDownloader->get(QNetworkRequest(backdropUrl));
    } else {
        backdropLabel->setText("No backdrop image");
        backdropLabel->setAlignment(Qt::AlignCenter);
        backdropLabel->setStyleSheet(QString("background-color: #2c2c2c; color: %1; border-radius: 12px;").arg(AppTheme::TEXT_ON_DARK));
    }

    // Title and Overlay Content Widget
    QWidget* overlayContentWidget = new QWidget();
    overlayContentWidget->setStyleSheet("background-color: transparent;"); // Ensure overlay is transparent
    QVBoxLayout* overlayLayout = new QVBoxLayout(overlayContentWidget);
    overlayLayout->setContentsMargins(20, 20, 20, 20); // Padding for text within overlay
    overlayLayout->addStretch(); // Push content to the bottom

    QString heroTitleText;
    if (itemData.contains("title") && !itemData["title"].toString().isEmpty()) {
        heroTitleText = itemData["title"].toString();
    } else {
        heroTitleText = itemData["name"].toString(); // For TV
    }

    QLabel *titleLabel = new QLabel(heroTitleText);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold; background-color: rgba(0,0,0,0.6); padding: 10px 15px; border-radius: 8px;")
                                  .arg(AppTheme::TEXT_ON_DARK));
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

    // Optional: Overview Snippet
    QLabel *overviewLabel = new QLabel(itemData["overview"].toString());
    overviewLabel->setStyleSheet(QString("color: %1; font-size: 14px; background-color: rgba(0,0,0,0.5); padding: 8px 12px; border-radius: 6px; margin-top: 5px;")
                                     .arg(AppTheme::TEXT_ON_DARK_SECONDARY)); // Assuming a secondary text color
    overviewLabel->setWordWrap(true);
    overviewLabel->setMaximumHeight(overviewLabel->fontMetrics().lineSpacing() * 3); // Limit to 3 lines

    overlayLayout->addWidget(titleLabel);
    overlayLayout->addWidget(overviewLabel);
    // Add more elements like buttons to overlayLayout if needed

    stackedHeroContentLayout->addWidget(backdropLabel); // Backdrop is at the bottom of the stack
    stackedHeroContentLayout->addWidget(overlayContentWidget); // Text overlay on top

    heroOuterLayout->addLayout(stackedHeroContentLayout);

    // Ensure hero banner is always at the top if m_scrollContentLayout is already populated
    if(m_scrollContentLayout->count() > 0) {
        m_scrollContentLayout->insertWidget(0, heroContainer);
    } else {
        m_scrollContentLayout->addWidget(heroContainer);
    }
}


QWidget* HomePage::createSection(const QString &title, const QString& sectionType)
{
    QWidget *sectionContainer = new QWidget(m_scrollContentWidget);
    sectionContainer->setObjectName("SectionContainer_" + title.simplified().replace(" ", "_"));
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionContainer);
    sectionLayout->setContentsMargins(0, 0, 0, 10); // Bottom margin for separation
    sectionLayout->setSpacing(12);

    QLabel *sectionLabel = new QLabel(title, sectionContainer);
    sectionLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold; margin-bottom: 5px;")
                                    .arg(AppTheme::TEXT_ON_DARK));
    sectionLayout->addWidget(sectionLabel);

    QScrollArea *horizontalScroll = new QScrollArea(sectionContainer);
    horizontalScroll->setWidgetResizable(true);
    horizontalScroll->setFrameShape(QFrame::NoFrame);
    horizontalScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    horizontalScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    horizontalScroll->setStyleSheet("background: transparent; border: none;");
    // MovieCard height is 300px, add some padding/spacing.
    // Min height of scroll area should accommodate the cards + a little bit of vertical padding within the scroll.
    horizontalScroll->setMinimumHeight(330); // MovieCard (300) + internal padding (e.g. 15 top/bottom for itemLayout)

    QWidget *itemContainerWidget = new QWidget();
    itemContainerWidget->setObjectName("ItemContainerFor_" + title.simplified().replace(" ", "_"));
    QHBoxLayout *itemLayout = new QHBoxLayout(itemContainerWidget);
    itemLayout->setContentsMargins(0, 5, 0, 5); // Padding around the row of cards
    itemLayout->setSpacing(20); // Space between MovieCards
    itemContainerWidget->setStyleSheet("background: transparent;");

    horizontalScroll->setWidget(itemContainerWidget);
    sectionLayout->addWidget(horizontalScroll);

    m_scrollContentLayout->addWidget(sectionContainer);
    return sectionContainer;
}

void HomePage::populateSection(QWidget* sectionWidget, const QJsonArray &items, const QString& itemType) {
    if (!sectionWidget) {
        qDebug() << "Section widget is null, cannot populate for type" << itemType;
        return;
    }

    QScrollArea* horizontalScroll = sectionWidget->findChild<QScrollArea*>();
    if (!horizontalScroll || !horizontalScroll->widget()) {
        qDebug() << "Horizontal scroll or its content widget not found for" << sectionWidget->objectName();
        return;
    }
    QHBoxLayout* itemLayout = qobject_cast<QHBoxLayout*>(horizontalScroll->widget()->layout());
    if(!itemLayout){
        qDebug() << "Item layout (QHBoxLayout) not found for section" << sectionWidget->objectName();
        return;
    }

    // Clear previous items in this specific section's layout
    QLayoutItem* child;
    while ((child = itemLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    int maxItemsToShow = 15;

    for (int i = 0; i < qMin(maxItemsToShow, items.size()); ++i) {
        QJsonObject itemData = items[i].toObject();
        MovieCard *card = new MovieCard(itemData, m_tmdbImageBaseUrl, m_tmdbPosterSize, horizontalScroll->widget());
        connect(card, &MovieCard::clicked, this, &HomePage::onMovieCardClicked);  // Add this
        itemLayout->addWidget(card);
    }

    itemLayout->addStretch(); // Ensures cards align left
}

void HomePage::onMovieCardClicked(const QJsonObject &itemData) {
    QString type = itemData.contains("title") ? "movie" : "tv";  // Determine if it's movie or TV
    int id = itemData["id"].toInt();
    emit movieClicked(id, type);
}

void HomePage::clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem *child;
    // Iterate backwards when removing items to avoid issues with index shifting
    while ((child = layout->takeAt(layout->count() - 1)) != nullptr) {
        if (child->layout()) {
            clearLayout(child->layout());
            delete child->layout(); // Delete the layout itself
        }
        if (child->widget()) {
            child->widget()->deleteLater(); // Defer deletion of widgets
        }
        delete child; // Delete the layout item
    }
}
