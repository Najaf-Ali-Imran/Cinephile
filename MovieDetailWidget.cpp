#include "MovieDetailWidget.h"
#include "Theme.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QFontMetrics>
#include <QStyleOption>
#include <QPainter>
#include <QFont>
#include <QMouseEvent>
#include <QMovie>
#include <QMessageBox>
#include <QDateTime>
#include <QToolTip>
#include <QNetworkReply>
#include <QDebug>
#include <qpainterpath.h>
#include <QLabel>
#include <QScrollBar>
#include <qscrollarea.h>
#include <QLayoutItem>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>

MovieDetailWidget::MovieDetailWidget(const QString &apiKey, QWidget *parent)
    : QWidget(parent), m_apiKey(apiKey), m_loading(false)
{
    // Initialize network managers with parent for automatic cleanup
    m_detailNetworkManager = new QNetworkAccessManager(this);
    m_imageNetworkManager = new QNetworkAccessManager(this);
    m_recommendationsNetworkManager = new QNetworkAccessManager(this);
    m_videosNetworkManager = new QNetworkAccessManager(this);
    m_providersNetworkManager = new QNetworkAccessManager(this);

    // Image configuration with multiple sizes for different use cases
    m_imageBaseUrl = "https://image.tmdb.org/t/p/";
    m_backdropSize = "w1280";  // Higher resolution for better quality
    m_posterSize = "w780";     // Larger poster size for better detail
    m_profileSize = "w250";    // For cast member photos
    m_logoSize = "w500";       // For streaming provider logos

    // Setup the UI
    setupUI();

    // Safe signal connections
    connect(m_detailNetworkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailWidget::handleDetailNetworkResponse);
    connect(m_imageNetworkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailWidget::handleImageNetworkResponse);
    connect(m_recommendationsNetworkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailWidget::handleRecommendationsResponse);
    connect(m_videosNetworkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailWidget::handleVideosResponse);
    connect(m_providersNetworkManager, &QNetworkAccessManager::finished,
            this, &MovieDetailWidget::handleProvidersResponse);
}

MovieDetailWidget::~MovieDetailWidget()
{
    // Disconnect all signals
    disconnect();

    // Clear network managers
    if(m_detailNetworkManager) m_detailNetworkManager->deleteLater();
    if(m_imageNetworkManager) m_imageNetworkManager->deleteLater();
    if(m_recommendationsNetworkManager) m_recommendationsNetworkManager->deleteLater();
    if(m_videosNetworkManager) m_videosNetworkManager->deleteLater();
    if(m_providersNetworkManager) m_providersNetworkManager->deleteLater();
}

void MovieDetailWidget::safeDeleteLayoutItems(QLayout *layout)
{
    if(!layout) return;

    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        if(child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void MovieDetailWidget::loadDetails(int itemId, const QString& itemType)
{
    if(itemId <= 0 || itemType.isEmpty()) return;

    // Reset current UI state
    if(m_titleLabel) m_titleLabel->setText("Loading...");
    if(m_taglineLabel) m_taglineLabel->setText("");
    if(m_overviewLabel) m_overviewLabel->setText("");
    if(m_metadataLabel) m_metadataLabel->setText("");
    if(m_ratingLabel) m_ratingLabel->setText("");
    if(m_releaseDateLabel) m_releaseDateLabel->setText("");
    if(m_directorLabel) m_directorLabel->setText("");

    // Clear existing content
    safeDeleteLayoutItems(m_genresLayout);
    safeDeleteLayoutItems(m_castLayout);
    safeDeleteLayoutItems(m_videosLayout);
    safeDeleteLayoutItems(m_recommendationsLayout);
    safeDeleteLayoutItems(m_providersLayout);

    // Store current item details
    m_currentItemId = itemId;
    m_currentItemType = itemType;

    // Fetch data
    fetchDetailData(itemId, itemType);
    fetchVideos(itemId, itemType);
    fetchRecommendations(itemId, itemType);
    fetchWatchProviders(itemId, itemType);

    m_loading = true;
}

void MovieDetailWidget::loadBackdropImage(const QString &fullBackdropPath)
{
    if(fullBackdropPath.isEmpty() || !m_imageNetworkManager) return;

    QNetworkRequest request;
    request.setUrl(QUrl(fullBackdropPath));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply *reply = m_imageNetworkManager->get(request);
    connect(reply, &QNetworkReply::errorOccurred, this, [](QNetworkReply::NetworkError error) {
        qDebug() << "Backdrop load error:" << error;
    });
}

void MovieDetailWidget::loadPosterImage(const QString &fullPosterPath)
{
    if(fullPosterPath.isEmpty() || !m_imageNetworkManager) return;

    QNetworkRequest request;
    request.setUrl(QUrl(fullPosterPath));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply *reply = m_imageNetworkManager->get(request);
    connect(reply, &QNetworkReply::errorOccurred, this, [](QNetworkReply::NetworkError error) {
        qDebug() << "Poster load error:" << error;
    });
}

void MovieDetailWidget::createRecommendationItem(const QJsonObject &itemData)
{
    if(itemData.isEmpty()) return;

    // Get data
    int itemId = itemData["id"].toInt();
    if(itemId <= 0) return;

    QString itemType;
    QString title;
    QString releaseDate;

    if (itemData.contains("title")) {
        title = itemData["title"].toString();
        itemType = "movie";
        releaseDate = itemData.contains("release_date") ? itemData["release_date"].toString() : "";
    } else if (itemData.contains("name")) {
        title = itemData["name"].toString();
        itemType = "tv";
        releaseDate = itemData.contains("first_air_date") ? itemData["first_air_date"].toString() : "";
    } else {
        return;
    }

    QWidget *itemWidget = new QWidget();
    itemWidget->setFixedSize(180, 280); // Increased size for better display
    itemWidget->setProperty("isRecommendation", true);
    itemWidget->setProperty("itemId", itemId);
    itemWidget->setProperty("itemType", itemType);
    itemWidget->setCursor(Qt::PointingHandCursor);
    itemWidget->setStyleSheet("QWidget:hover { background-color: " + AppTheme::INTERACTIVE_HOVER_BG + "; border-radius: 8px; }");
    itemWidget->installEventFilter(this);

    QVBoxLayout *itemLayout = new QVBoxLayout(itemWidget);
    itemLayout->setContentsMargins(5, 5, 5, 5);
    itemLayout->setSpacing(8);

    QLabel *posterLabel = new QLabel(itemWidget);
    posterLabel->setFixedSize(170, 170 * 1.5); // Better aspect ratio
    posterLabel->setStyleSheet("QLabel { background-color: " + AppTheme::SKELETON_BG +
                               "; border-radius: 8px; border: 1px solid " +
                               AppTheme::INTERACTIVE_HOVER_BG + "; }");
    posterLabel->setAlignment(Qt::AlignCenter);
    posterLabel->setText("Loading...");

    if (itemData.contains("poster_path") && !itemData["poster_path"].isNull()) {
        QString posterPath = itemData["poster_path"].toString();
        if (!posterPath.isEmpty()) {
            QString fullPosterPath = m_imageBaseUrl + "w342" + posterPath;
            QNetworkRequest request;
            request.setUrl(QUrl(fullPosterPath));
            QNetworkReply *reply = m_imageNetworkManager->get(request);

            connect(reply, &QNetworkReply::finished, [reply, posterLabel]() {
                if (!reply || !posterLabel) {
                    if(reply) reply->deleteLater();
                    return;
                }

                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray imageData = reply->readAll();
                    if(!imageData.isEmpty()) {
                        QPixmap pixmap;
                        if(pixmap.loadFromData(imageData)) {
                            pixmap = pixmap.scaled(posterLabel->size(),
                                                   Qt::KeepAspectRatioByExpanding,
                                                   Qt::SmoothTransformation);
                            posterLabel->setPixmap(pixmap);
                        }
                    }
                }
                reply->deleteLater();
            });
        }
    }

    QLabel *titleLabel = new QLabel(title, itemWidget);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    titleLabel->setStyleSheet("QLabel { color: " + AppTheme::TEXT_ON_DARK + "; font-size: 12px; font-weight: bold; }");

    QFontMetrics fm(titleLabel->font());
    QString elidedTitle = fm.elidedText(title, Qt::ElideRight, 160);
    titleLabel->setText(elidedTitle);
    titleLabel->setToolTip(title);

    // Add rating if available
    if (itemData.contains("vote_average") && itemData["vote_average"].toDouble() > 0) {
        QLabel *ratingLabel = new QLabel(itemWidget);
        double voteAverage = itemData["vote_average"].toDouble();
        ratingLabel->setText(QString::number(voteAverage, 'f', 1) + " ★");
        ratingLabel->setStyleSheet("QLabel { color: " + AppTheme::PRIMARY_RED + "; font-size: 11px; }");
        ratingLabel->setAlignment(Qt::AlignLeft);
        itemLayout->addWidget(ratingLabel);
    }

    // Add release year if available
    if (!releaseDate.isEmpty()) {
        QLabel *yearLabel = new QLabel(itemWidget);
        QDate date = QDate::fromString(releaseDate.left(10), "yyyy-MM-dd");
        yearLabel->setText(date.isValid() ? date.toString("yyyy") : "");
        yearLabel->setStyleSheet("QLabel { color: " + AppTheme::TEXT_ON_DARK_SECONDARY + "; font-size: 11px; }");
        yearLabel->setAlignment(Qt::AlignLeft);
        itemLayout->addWidget(yearLabel);
    }

    itemLayout->addWidget(posterLabel);
    itemLayout->addWidget(titleLabel);

    if(m_recommendationsLayout) {
        m_recommendationsLayout->addWidget(itemWidget);
    } else {
        itemWidget->deleteLater();
    }
}

void MovieDetailWidget::createTagPill(const QString &text, QLayout *targetLayout)
{
    if(text.isEmpty() || !targetLayout) return;

    QLabel *pill = new QLabel(text);
    pill->setStyleSheet(
        "QLabel {"
        "   background-color: " + AppTheme::PRIMARY_RED_ALPHA_15 + ";"
        "   color: " + AppTheme::PRIMARY_RED + ";"
        "   padding: 4px 12px;"
        "   border-radius: 12px;"
        "   font-size: 12px;"
        "}"
    );
    pill->setFixedHeight(24);
    targetLayout->addWidget(pill);
}

bool MovieDetailWidget::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget*>(watched);
    if (!widget || !widget->property("isRecommendation").toBool()) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::Enter) {
        widget->setStyleSheet("background-color: " + AppTheme::INTERACTIVE_HOVER_BG + "; border-radius: 8px;");
        return true;
    }
    else if (event->type() == QEvent::Leave) {
        widget->setStyleSheet("background-color: transparent; border-radius: 8px;");
        return true;
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        int itemId = widget->property("itemId").toInt();
        QString itemType = widget->property("itemType").toString();
        if(itemId > 0 && !itemType.isEmpty()) {
            loadDetails(itemId, itemType);
            // Scroll to top when clicking a recommendation
            if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
                m_scrollArea->verticalScrollBar()->setValue(0);
            }
        }
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void MovieDetailWidget::handleDetailNetworkResponse(QNetworkReply *reply)
{
    if(!reply || !m_titleLabel || !m_overviewLabel) {
        if(reply) reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);

        if (!document.isNull() && document.isObject()) {
            QJsonObject jsonResponse = document.object();
            populateUI(jsonResponse);

            if (jsonResponse.contains("backdrop_path") && !jsonResponse["backdrop_path"].isNull()) {
                QString backdropPath = jsonResponse["backdrop_path"].toString();
                if (!backdropPath.isEmpty()) {
                    loadBackdropImage(m_imageBaseUrl + m_backdropSize + backdropPath);
                }
            }

            if (jsonResponse.contains("poster_path") && !jsonResponse["poster_path"].isNull()) {
                QString posterPath = jsonResponse["poster_path"].toString();
                if (!posterPath.isEmpty()) {
                    loadPosterImage(m_imageBaseUrl + m_posterSize + posterPath);
                }
            }
        }
    } else {
        qDebug() << "Network Error:" << reply->errorString();
        if(m_titleLabel) m_titleLabel->setText("Error loading details");
        if(m_overviewLabel) m_overviewLabel->setText("Failed to load content. Please check your network connection.");
    }

    m_loading = false;
    reply->deleteLater();
}

void MovieDetailWidget::handleImageNetworkResponse(QNetworkReply *reply)
{
    if(!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        if(imageData.isEmpty()) {
            reply->deleteLater();
            return;
        }

        QPixmap pixmap;
        if(!pixmap.loadFromData(imageData)) {
            reply->deleteLater();
            return;
        }

        QString url = reply->url().toString();
        if (url.contains(m_backdropSize) && m_backdropLabel) {
            // Add gradient overlay to backdrop
            QPixmap scaled = pixmap.scaledToWidth(width(), Qt::SmoothTransformation);

            // Create a new pixmap with gradient overlay
            QPixmap finalPixmap(scaled.size());
            finalPixmap.fill(Qt::transparent);

            QPainter painter(&finalPixmap);
            painter.drawPixmap(0, 0, scaled);

            // Add gradient overlay
            QLinearGradient gradient(0, 0, 0, scaled.height());
            gradient.setColorAt(0, QColor(0, 0, 0, 150));
            gradient.setColorAt(1, QColor(0, 0, 0, 50));
            painter.fillRect(scaled.rect(), gradient);

            m_backdropLabel->setPixmap(finalPixmap);
            m_backdropLabel->setMinimumHeight(scaled.height());
        }
        else if (url.contains(m_posterSize) && m_posterLabel) {
            QPixmap scaled = pixmap.scaled(m_posterLabel->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);

            // Create rounded corners
            QPixmap roundedPixmap(m_posterLabel->size());
            roundedPixmap.fill(Qt::transparent);

            QPainter painter(&roundedPixmap);
            painter.setRenderHint(QPainter::Antialiasing);

            QPainterPath path;
            path.addRoundedRect(0, 0, scaled.width(), scaled.height(), 8, 8);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaled);

            // Add subtle drop shadow
            painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
            QPainterPath shadowPath;
            shadowPath.addRoundedRect(QRectF(2, 2, scaled.width()-4, scaled.height()-4), 6, 6);
            painter.fillPath(shadowPath, QColor(0, 0, 0, 50));

            m_posterLabel->setPixmap(roundedPixmap);
        }
        else if (url.contains(m_profileSize) && reply->request().hasRawHeader("X-Cast-Photo")) {
            // Handle cast member photos
            int castIndex = reply->request().rawHeader("X-Cast-Index").toInt();
            QWidget *castWidget = m_castLayout->itemAt(castIndex)->widget();
            if (castWidget) {
                QLabel *photoLabel = castWidget->findChild<QLabel*>("castPhoto");
                if (photoLabel) {
                    QPixmap rounded = createRoundedPixmap(pixmap.scaled(QSize(50, 50), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    photoLabel->setPixmap(rounded);
                }
            }
        }
        else if (url.contains(m_logoSize) && reply->request().hasRawHeader("X-Provider-Logo")) {
            // Handle provider logos
            int providerIndex = reply->request().rawHeader("X-Provider-Index").toInt();
            QWidget *providerWidget = m_providersLayout->itemAt(providerIndex)->widget();
            if (providerWidget) {
                QLabel *logoLabel = providerWidget->findChild<QLabel*>("providerLogo");
                if (logoLabel) {
                    QPixmap scaled = pixmap.scaled(QSize(45, 45), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    logoLabel->setPixmap(scaled);
                }
            }
        }
    } else {
        qDebug() << "Image Network Error:" << reply->errorString();
    }

    reply->deleteLater();
}

QPixmap MovieDetailWidget::createRoundedPixmap(const QPixmap &source)
{
    QPixmap result(source.size());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addEllipse(result.rect());
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, source);

    return result;
}

void MovieDetailWidget::handleRecommendationsResponse(QNetworkReply *reply)
{
    if(!reply || !m_recommendationsLabel) {
        if(reply) reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);

        if (!document.isNull() && document.isObject()) {
            QJsonObject jsonResponse = document.object();

            if (jsonResponse.contains("results") && jsonResponse["results"].isArray()) {
                QJsonArray results = jsonResponse["results"].toArray();

                if (results.isEmpty()) {
                    m_recommendationsLabel->setText("No recommendations available");
                } else {
                    m_recommendationsLabel->setText("You might also like");

                    int itemsToShow = qMin(10, results.size());
                    for (int i = 0; i < itemsToShow; i++) {
                        createRecommendationItem(results[i].toObject());
                    }
                }
            }
        }
    } else {
        qDebug() << "Recommendations Network Error:" << reply->errorString();
        m_recommendationsLabel->setText("Failed to load recommendations");
    }

    reply->deleteLater();
}

void MovieDetailWidget::handleVideosResponse(QNetworkReply *reply)
{
    if(!reply || !m_videosLabel) {
        if(reply) reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);

        if (!document.isNull() && document.isObject()) {
            QJsonObject jsonResponse = document.object();

            if (jsonResponse.contains("results") && jsonResponse["results"].isArray()) {
                QJsonArray results = jsonResponse["results"].toArray();

                if (results.isEmpty()) {
                    m_videosLabel->setText("No videos available");
                } else {
                    m_videosLabel->setText("Videos & Trailers");

                    // Sort videos: trailers first, then teasers, then others
                    QVector<QJsonObject> trailers;
                    QVector<QJsonObject> teasers;
                    QVector<QJsonObject> others;

                    for (const QJsonValue &value : results) {
                        QJsonObject video = value.toObject();
                        QString type = video["type"].toString().toLower();
                        if (type == "trailer") {
                            trailers.append(video);
                        } else if (type == "teaser") {
                            teasers.append(video);
                        } else {
                            others.append(video);
                        }
                    }

                    // Combine with trailers first
                    QVector<QJsonObject> sortedVideos;
                    sortedVideos << trailers << teasers << others;

                    int itemsToShow = qMin(5, sortedVideos.size());
                    for (int i = 0; i < itemsToShow; i++) {
                        QJsonObject videoData = sortedVideos[i];
                        QString videoSite = videoData["site"].toString().toLower();
                        if (videoSite != "youtube") continue;

                        QString videoName = videoData["name"].toString();
                        QString videoKey = videoData["key"].toString();
                        QString videoType = videoData["type"].toString();

                        // Create video thumbnail widget
                        QWidget *videoWidget = new QWidget();
                        videoWidget->setFixedSize(320, 180);

                        QVBoxLayout *videoLayout = new QVBoxLayout(videoWidget);
                        videoLayout->setContentsMargins(0, 0, 0, 0);
                        videoLayout->setSpacing(5);

                        // Thumbnail with play button overlay
                        QLabel *thumbnailLabel = new QLabel();
                        thumbnailLabel->setFixedSize(320, 180);
                        thumbnailLabel->setAlignment(Qt::AlignCenter);
                        thumbnailLabel->setStyleSheet(
                            "QLabel {"
                            "   background-color: " + AppTheme::SKELETON_BG + ";"
                            "   border-radius: 4px;"
                            "}"
                        );

                        // Load YouTube thumbnail
                        QLabel *thumbnail = new QLabel();
                        thumbnail->setFixedSize(320, 180);
                        thumbnail->setAlignment(Qt::AlignCenter);
                        thumbnail->setStyleSheet("border-radius: 4px;");

                        QString thumbnailUrl = QString("https://img.youtube.com/vi/%1/hqdefault.jpg").arg(videoKey);
                        QNetworkRequest request;
                        request.setUrl(QUrl(thumbnailUrl));
                        QNetworkReply *thumbnailReply = m_imageNetworkManager->get(request);

                        connect(thumbnailReply, &QNetworkReply::finished, [thumbnailReply, thumbnail]() {
                            if (thumbnailReply->error() == QNetworkReply::NoError) {
                                QByteArray imageData = thumbnailReply->readAll();
                                QPixmap pixmap;
                                if (pixmap.loadFromData(imageData)) {
                                    pixmap = pixmap.scaled(thumbnail->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                                    thumbnail->setPixmap(pixmap);
                                }
                            }
                            thumbnailReply->deleteLater();
                        });

                        // Play button overlay
                        QLabel *playButton = new QLabel();
                        playButton->setFixedSize(60, 60);
                        playButton->setAlignment(Qt::AlignCenter);
                        playButton->setStyleSheet(
                            "QLabel {"
                            "   background-color: rgba(255, 255, 255, 0.8);"
                            "   border-radius: 30px;"
                            "}"
                        );
                        playButton->setPixmap(QPixmap(":/icons/play.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));

                        // Video title
                        QLabel *titleLabel = new QLabel(videoName);
                        titleLabel->setStyleSheet(
                            "QLabel {"
                            "   color: " + AppTheme::TEXT_ON_DARK + ";"
                            "   font-size: 13px;"
                            "   font-weight: bold;"
                            "}"
                        );
                        titleLabel->setWordWrap(true);
                        titleLabel->setMaximumWidth(320);

                        // Video type badge
                        QLabel *typeBadge = new QLabel(videoType);
                        typeBadge->setStyleSheet(
                            "QLabel {"
                            "   background-color: " + AppTheme::PRIMARY_RED_ALPHA_15 + ";"
                            "   color: " + AppTheme::PRIMARY_RED + ";"
                            "   padding: 2px 8px;"
                            "   border-radius: 10px;"
                            "   font-size: 10px;"
                            "}"
                        );
                        typeBadge->setAlignment(Qt::AlignCenter);
                        typeBadge->setFixedHeight(20);

                        // Overlay layout for thumbnail
                        QWidget *thumbnailOverlay = new QWidget();
                        thumbnailOverlay->setLayout(new QVBoxLayout());
                        thumbnailOverlay->layout()->setAlignment(playButton, Qt::AlignCenter);
                        thumbnailOverlay->setStyleSheet("background-color: transparent;");

                        // Stack thumbnail and overlay
                        QStackedWidget *stackedThumbnail = new QStackedWidget();
                        stackedThumbnail->addWidget(thumbnail);
                        stackedThumbnail->addWidget(thumbnailOverlay);
                        stackedThumbnail->setCurrentIndex(0);
                        stackedThumbnail->setStyleSheet("QStackedWidget { background-color: transparent; border-radius: 4px; }");

                        // Connect click event
                        connect(thumbnail, &QLabel::mousePressEvent, [videoKey]() {
                            QDesktopServices::openUrl(QUrl("https://www.youtube.com/watch?v=" + videoKey));
                        });
                        thumbnail->setCursor(Qt::PointingHandCursor);

                        videoLayout->addWidget(stackedThumbnail);
                        videoLayout->addWidget(titleLabel);
                        videoLayout->addWidget(typeBadge, 0, Qt::AlignLeft);

                        if(m_videosLayout) {
                            m_videosLayout->addWidget(videoWidget);
                        } else {
                            videoWidget->deleteLater();
                        }
                    }
                }
            }
        }
    } else {
        qDebug() << "Videos Network Error:" << reply->errorString();
        m_videosLabel->setText("Failed to load videos");
    }

    reply->deleteLater();
}

void MovieDetailWidget::handleProvidersResponse(QNetworkReply *reply)
{
    if(!reply || !m_providersLabel) {
        if(reply) reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);

        if (!document.isNull() && document.isObject()) {
            QJsonObject jsonResponse = document.object();

            // Get country-specific providers (using US as default)
            QString countryCode = "US";
            if (jsonResponse.contains("results") && jsonResponse["results"].isObject()) {
                QJsonObject countries = jsonResponse["results"].toObject();

                if (countries.contains(countryCode)) {
                    QJsonObject countryProviders = countries[countryCode].toObject();

                    // Flatrate (subscription) providers
                    if (countryProviders.contains("flatrate") && countryProviders["flatrate"].isArray()) {
                        QJsonArray providers = countryProviders["flatrate"].toArray();

                        if (providers.isEmpty()) {
                            m_providersLabel->setText("Not available on any streaming service");
                        } else {
                            m_providersLabel->setText("Available on");

                            for (const QJsonValue &providerValue : providers) {
                                QJsonObject provider = providerValue.toObject();
                                QString providerName = provider["provider_name"].toString();
                                QString logoPath = provider["logo_path"].toString();

                                if (!logoPath.isEmpty()) {
                                    QWidget *providerWidget = new QWidget();
                                    providerWidget->setFixedSize(60, 80);

                                    QVBoxLayout *providerLayout = new QVBoxLayout(providerWidget);
                                    providerLayout->setContentsMargins(0, 0, 0, 0);
                                    providerLayout->setSpacing(5);
                                    providerLayout->setAlignment(Qt::AlignCenter);

                                    QLabel *logoLabel = new QLabel();
                                    logoLabel->setObjectName("providerLogo");
                                    logoLabel->setFixedSize(45, 45);
                                    logoLabel->setAlignment(Qt::AlignCenter);
                                    logoLabel->setStyleSheet("background-color: white; border-radius: 8px; padding: 2px;");

                                    QLabel *nameLabel = new QLabel(providerName);
                                    nameLabel->setStyleSheet(
                                        "QLabel {"
                                        "   color: " + AppTheme::TEXT_ON_DARK + ";"
                                        "   font-size: 11px;"
                                        "}"
                                    );
                                    nameLabel->setAlignment(Qt::AlignCenter);
                                    nameLabel->setWordWrap(true);

                                    providerLayout->addWidget(logoLabel);
                                    providerLayout->addWidget(nameLabel);

                                    // Load logo
                                    QString fullLogoPath = m_imageBaseUrl + m_logoSize + logoPath;
                                    QNetworkRequest request;
                                    request.setUrl(QUrl(fullLogoPath));
                                    request.setRawHeader("X-Provider-Logo", "1");
                                    request.setRawHeader("X-Provider-Index", QByteArray::number(m_providersLayout->count()));
                                    m_imageNetworkManager->get(request);

                                    m_providersLayout->addWidget(providerWidget);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        qDebug() << "Providers Network Error:" << reply->errorString();
        m_providersLabel->setText("Streaming availability unknown");
    }

    reply->deleteLater();
}

void MovieDetailWidget::setupUI()
{
    // Main widget setup
    setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");

    // Create scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: " + AppTheme::MAIN_WINDOW_BG + "; }"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: transparent;"
        "   width: 8px;"
        "   margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: " + AppTheme::INTERACTIVE_HOVER_BG + ";"
        "   min-height: 20px;"
        "   border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
    );

    // Create content widget
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
    m_contentWidget->setMinimumWidth(this->width());

    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Backdrop image
    m_backdropLabel = new QLabel();
    m_backdropLabel->setScaledContents(false);
    m_backdropLabel->setAlignment(Qt::AlignCenter);
    m_backdropLabel->setStyleSheet("QLabel { background-color: " + AppTheme::BACKGROUND_DARK + "; }");
    m_backdropLabel->setMinimumHeight(400);

    // Content container
    QWidget *contentContainer = new QWidget();
    contentContainer->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(40, 30, 40, 40);
    contentLayout->setSpacing(30);

    // Header with back button
    QWidget *headerWidget = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    m_backButton = new QPushButton("← Back to Results");
    m_backButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   border: none;"
        "   font-size: 16px;"
        "   padding: 5px 0;"
        "}"
        "QPushButton:hover {"
        "   color: " + AppTheme::PRIMARY_RED + ";"
        "}"
    );
    m_backButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_backButton);
    headerLayout->addStretch();

    // Main content area
    QWidget *mainContentWidget = new QWidget();
    QHBoxLayout *mainContentLayout = new QHBoxLayout(mainContentWidget);
    mainContentLayout->setContentsMargins(0, 0, 0, 0);
    mainContentLayout->setSpacing(30);

    // Poster column
    m_posterLabel = new QLabel();
    m_posterLabel->setFixedSize(260, 390); // Larger poster size
    m_posterLabel->setStyleSheet(
        "QLabel {"
        "   background-color: " + AppTheme::SKELETON_BG + ";"
        "   border-radius: 8px;"
        "   border: 1px solid " + AppTheme::INTERACTIVE_HOVER_BG + ";"
        "}"
    );
    m_posterLabel->setAlignment(Qt::AlignCenter);
    m_posterLabel->setText("No Poster");

    // Details column
    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(15);

    // Title
    m_titleLabel = new QLabel("Loading...");
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 36px;"
        "   font-weight: bold;"
        "   letter-spacing: -0.5px;"
        "}"
    );
    m_titleLabel->setWordWrap(true);

    // Tagline
    m_taglineLabel = new QLabel();
    m_taglineLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK_SECONDARY + ";"
        "   font-style: italic;"
        "   font-size: 16px;"
        "}"
    );
    m_taglineLabel->setWordWrap(true);

    // Metadata row
    QWidget *metadataWidget = new QWidget();
    m_metadataLabel = new QLabel();
    m_releaseDateLabel = new QLabel();
    m_ratingLabel = new QLabel();

    QHBoxLayout *metadataLayout = new QHBoxLayout(metadataWidget);
    metadataLayout->setContentsMargins(0, 0, 0, 0);
    metadataLayout->setSpacing(15);

    QFont metadataFont;
    metadataFont.setPointSize(13);

    m_releaseDateLabel->setFont(metadataFont);
    m_ratingLabel->setFont(metadataFont);
    m_metadataLabel->setFont(metadataFont);

    m_releaseDateLabel->setStyleSheet("QLabel { color: " + AppTheme::TEXT_ON_DARK_SECONDARY + "; }");
    m_ratingLabel->setStyleSheet("QLabel { color: " + AppTheme::TEXT_ON_DARK_SECONDARY + "; }");
    m_metadataLabel->setStyleSheet("QLabel { color: " + AppTheme::TEXT_ON_DARK_SECONDARY + "; }");

    metadataLayout->addWidget(m_releaseDateLabel);
    metadataLayout->addWidget(m_ratingLabel);
    metadataLayout->addWidget(m_metadataLabel);
    metadataLayout->addStretch();

    // Genres
    QWidget *genresWidget = new QWidget();
    m_genresLayout = new QHBoxLayout(genresWidget);
    m_genresLayout->setContentsMargins(0, 0, 0, 0);
    m_genresLayout->setSpacing(8);

    // Action buttons
    QWidget *actionsWidget = new QWidget();
    m_playButton = new QPushButton("▶ Play");
    m_addToListButton = new QPushButton("+ Add to Watchlist");
    m_watchTrailerButton = new QPushButton("Watch Trailer");

    QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(15);

    m_playButton->setStyleSheet(
        "QPushButton {"
        "   background-color: " + AppTheme::PRIMARY_RED + ";"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 10px 25px;"
        "   border-radius: 6px;"
        "   min-width: 120px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: " + AppTheme::BUTTON_HOVER_LIGHT_BG + ";"
        "}"
    );
    m_playButton->setCursor(Qt::PointingHandCursor);

    m_addToListButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   padding: 10px 25px;"
        "   border: 1px solid " + AppTheme::TEXT_ON_DARK_SECONDARY + ";"
        "   border-radius: 6px;"
        "   min-width: 120px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   border-color: " + AppTheme::TEXT_ON_DARK + ";"
        "}"
    );
    m_addToListButton->setCursor(Qt::PointingHandCursor);

    m_watchTrailerButton->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   padding: 10px 25px;"
        "   border: 1px solid " + AppTheme::TEXT_ON_DARK_SECONDARY + ";"
        "   border-radius: 6px;"
        "   min-width: 120px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   border-color: " + AppTheme::TEXT_ON_DARK + ";"
        "}"
    );
    m_watchTrailerButton->setCursor(Qt::PointingHandCursor);

    actionsLayout->addWidget(m_playButton);
    actionsLayout->addWidget(m_addToListButton);
    actionsLayout->addWidget(m_watchTrailerButton);
    actionsLayout->addStretch();

    // Overview
    m_overviewLabel = new QLabel();
    m_overviewLabel->setWordWrap(true);
    m_overviewLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 16px;"
        "   line-height: 1.6;"
        "   margin-top: 10px;"
        "}"
    );

    // Director info
    m_directorLabel = new QLabel();
    m_directorLabel->setWordWrap(true);
    m_directorLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 14px;"
        "   font-style: italic;"
        "}"
    );

    // Add to details layout
    detailsLayout->addWidget(m_titleLabel);
    detailsLayout->addWidget(m_taglineLabel);
    detailsLayout->addWidget(metadataWidget);
    detailsLayout->addWidget(genresWidget);
    detailsLayout->addWidget(actionsWidget);
    detailsLayout->addWidget(m_overviewLabel);
    detailsLayout->addWidget(m_directorLabel, 0, Qt::AlignLeft);
    detailsLayout->addStretch();

    // Add to main content layout
    mainContentLayout->addWidget(m_posterLabel);
    mainContentLayout->addWidget(detailsWidget, 1);

    // Streaming providers section
    m_providersLabel = new QLabel("Where to Watch");
    m_providersLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
    );

    QWidget *providersWidget = new QWidget();
    QVBoxLayout *providersContainerLayout = new QVBoxLayout(providersWidget);
    providersContainerLayout->setContentsMargins(0, 0, 0, 0);
    providersContainerLayout->setSpacing(15);

    // Create a flow layout for providers
    QWidget *providersFlowWidget = new QWidget();
    m_providersLayout = new QHBoxLayout(providersFlowWidget);
    m_providersLayout->setContentsMargins(0, 0, 0, 0);
    m_providersLayout->setSpacing(15);

    providersContainerLayout->addWidget(m_providersLabel);
    providersContainerLayout->addWidget(providersFlowWidget);

    // Cast section
    QLabel *castLabel = new QLabel("Cast");
    castLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
    );

    QWidget *castWidget = new QWidget();
    m_castLayout = new QVBoxLayout(castWidget);
    m_castLayout->setContentsMargins(0, 0, 0, 0);
    m_castLayout->setSpacing(12);

    // Videos section
    m_videosLabel = new QLabel("Videos & Trailers");
    m_videosLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
    );

    QWidget *videosWidget = new QWidget();
    QVBoxLayout *videosContainerLayout = new QVBoxLayout(videosWidget);
    videosContainerLayout->setContentsMargins(0, 0, 0, 0);
    videosContainerLayout->setSpacing(15);

    // Create a flow layout for videos
    QWidget *videosFlowWidget = new QWidget();
    m_videosLayout = new QHBoxLayout(videosFlowWidget);
    m_videosLayout->setContentsMargins(0, 0, 0, 0);
    m_videosLayout->setSpacing(20);

    // Create a scroll area for videos
    QScrollArea *videosScrollArea = new QScrollArea();
    videosScrollArea->setWidgetResizable(true);
    videosScrollArea->setFrameShape(QFrame::NoFrame);
    videosScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    videosScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    videosScrollArea->setMaximumHeight(1000);
    videosScrollArea->setStyleSheet("QScrollArea { "
                                    ""
                                    "border: none; background: transparent; }");
    videosScrollArea->setWidget(videosFlowWidget);

    videosContainerLayout->addWidget(m_videosLabel);
    videosContainerLayout->addWidget(videosScrollArea);

    // Recommendations section
    m_recommendationsLabel = new QLabel("Recommendations");
    m_recommendationsLabel->setStyleSheet(
        "QLabel {"
        "   color: " + AppTheme::TEXT_ON_DARK + ";"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
    );

    QWidget *recommendationsWidget = new QWidget();
    QVBoxLayout *recommendationsContainerLayout = new QVBoxLayout(recommendationsWidget);
    recommendationsContainerLayout->setContentsMargins(0, 0, 0, 0);
    recommendationsContainerLayout->setSpacing(15);

    // Create a flow layout for recommendations
    QWidget *recommendationsFlowWidget = new QWidget();
    m_recommendationsLayout = new QHBoxLayout(recommendationsFlowWidget);
    m_recommendationsLayout->setContentsMargins(0, 0, 0, 0);
    m_recommendationsLayout->setSpacing(20);

    // Create a scroll area for recommendations
    QScrollArea *recommendationsScrollArea = new QScrollArea();
    recommendationsScrollArea->setWidgetResizable(true);
    recommendationsScrollArea->setFrameShape(QFrame::NoFrame);
    recommendationsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    recommendationsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recommendationsScrollArea->setMaximumHeight(300);
    recommendationsScrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    recommendationsScrollArea->setWidget(recommendationsFlowWidget);

    recommendationsContainerLayout->addWidget(m_recommendationsLabel);
    recommendationsContainerLayout->addWidget(recommendationsScrollArea);

    // Add sections to content layout in correct order
    contentLayout->addWidget(headerWidget);
    contentLayout->addWidget(mainContentWidget);
    addSectionDivider(contentLayout);
    contentLayout->addWidget(providersWidget);
    addSectionDivider(contentLayout);
    contentLayout->addWidget(castLabel);
    contentLayout->addWidget(castWidget);
    addSectionDivider(contentLayout);
    contentLayout->addWidget(videosWidget);
    addSectionDivider(contentLayout);
    contentLayout->addWidget(recommendationsWidget);

    // Add main components to main layout
    m_mainLayout->addWidget(m_backdropLabel);
    m_mainLayout->addWidget(contentContainer);

    // Set the content widget to the scroll area
    m_scrollArea->setWidget(m_contentWidget);

    // Set up the main widget layout
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(this);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget(m_scrollArea);

    // Connect signals
    connect(m_backButton, &QPushButton::clicked, this, &MovieDetailWidget::onBackButtonClicked);
    connect(m_addToListButton, &QPushButton::clicked, this, &MovieDetailWidget::onAddToListClicked);
    connect(m_playButton, &QPushButton::clicked, this, &MovieDetailWidget::onPlayButtonClicked);
    connect(m_watchTrailerButton, &QPushButton::clicked, this, [this]() {
        if (m_videosLayout && m_videosLayout->count() > 0) {
            // Find the first trailer button and click it
            for (int i = 0; i < m_videosLayout->count(); ++i) {
                QWidget *widget = m_videosLayout->itemAt(i)->widget();
                if (widget) {
                    QLabel *thumbnail = widget->findChild<QLabel*>();
                    if (thumbnail) {
                        Q_EMIT thumbnail->mousePressEvent(nullptr);
                        break;
                    }
                }
            }
        }
    });

    // Set initial state
    m_playButton->setVisible(false);
    m_watchTrailerButton->setVisible(false);
}

void MovieDetailWidget::addSectionDivider(QLayout *layout) {
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: " + AppTheme::INTERACTIVE_HOVER_BG + ";");
    line->setFixedHeight(1);
    layout->addWidget(line);
}

void MovieDetailWidget::populateUI(const QJsonObject &detailData)
{
    if(detailData.isEmpty()) return;

    // Set title
    if (m_currentItemType == "movie") {
        m_currentTitle = detailData["title"].toString();
        if(m_titleLabel) m_titleLabel->setText(m_currentTitle);
    } else {
        m_currentTitle = detailData["name"].toString();
        if(m_titleLabel) m_titleLabel->setText(m_currentTitle);
    }

    // Set tagline
    if (detailData.contains("tagline") && !detailData["tagline"].toString().isEmpty()) {
        if(m_taglineLabel) {
            m_taglineLabel->setText(detailData["tagline"].toString());
            m_taglineLabel->setVisible(true);
        }
    } else if(m_taglineLabel) {
        m_taglineLabel->setVisible(false);
    }

    // Set overview
    if (detailData.contains("overview") && !detailData["overview"].toString().isEmpty()) {
        if(m_overviewLabel) m_overviewLabel->setText(detailData["overview"].toString());
    } else if(m_overviewLabel) {
        m_overviewLabel->setText("No overview available.");
    }

    // Set release date
    QString releaseDate;
    if (m_currentItemType == "movie" && detailData.contains("release_date")) {
        releaseDate = detailData["release_date"].toString();
        if (!releaseDate.isEmpty() && m_releaseDateLabel) {
            QDate date = QDate::fromString(releaseDate, "yyyy-MM-dd");
            m_releaseDateLabel->setText(date.toString("MMMM d, yyyy"));
        }
    } else if (m_currentItemType == "tv" && detailData.contains("first_air_date")) {
        releaseDate = detailData["first_air_date"].toString();
        if (!releaseDate.isEmpty() && m_releaseDateLabel) {
            QDate date = QDate::fromString(releaseDate, "yyyy-MM-dd");
            m_releaseDateLabel->setText(date.toString("MMMM d, yyyy"));
        }
    }

    // Set rating with color based on score
    if (detailData.contains("vote_average") && m_ratingLabel) {
        double voteAverage = detailData["vote_average"].toDouble();
        QString ratingColor = voteAverage >= 7.5 ? AppTheme::PRIMARY_RED :
                         voteAverage >= 5.0 ? AppTheme::TEXT_ON_DARK :
                         "#FF5555"; // Red for low ratings
        m_ratingLabel->setText(QString::number(voteAverage, 'f', 1) + " ★");
        m_ratingLabel->setStyleSheet("QLabel { color: " + ratingColor + "; }");
    }

    // Set metadata
    QStringList metadata;
    if (m_currentItemType == "movie") {
        if (detailData.contains("runtime") && detailData["runtime"].toInt() > 0) {
            int runtime = detailData["runtime"].toInt();
            metadata << QString("%1h %2m").arg(runtime / 60).arg(runtime % 60);
        }
    } else {
        if (detailData.contains("number_of_seasons")) {
            int seasons = detailData["number_of_seasons"].toInt();
            metadata << QString("%1 season%2").arg(seasons).arg(seasons > 1 ? "s" : "");
        }
        if (detailData.contains("number_of_episodes")) {
            int episodes = detailData["number_of_episodes"].toInt();
            metadata << QString("%1 episodes").arg(episodes);
        }
    }

    if (detailData.contains("genres") && detailData["genres"].isArray()) {
        QJsonArray genres = detailData["genres"].toArray();
        QStringList genreNames;
        for (const QJsonValue &genre : genres) {
            genreNames << genre.toObject()["name"].toString();
        }
        metadata << genreNames.join(", ");
    }

    if(m_metadataLabel) m_metadataLabel->setText(metadata.join(" • "));

    // Set genres
    if (detailData.contains("genres") && detailData["genres"].isArray() && m_genresLayout) {
        QJsonArray genres = detailData["genres"].toArray();
        for (const QJsonValue &genre : genres) {
            createTagPill(genre.toObject()["name"].toString(), m_genresLayout);
        }
    }

    // Process credits
    if (detailData.contains("credits")) {
        QJsonObject credits = detailData["credits"].toObject();

        // Director info
        if (credits.contains("crew") && credits["crew"].isArray() && m_directorLabel) {
            QJsonArray crew = credits["crew"].toArray();
            QStringList directors;

            for (const QJsonValue &member : crew) {
                QJsonObject job = member.toObject();
                if (job["job"].toString() == "Director") {
                    directors << job["name"].toString();
                }
            }

            if (!directors.isEmpty()) {
                m_directorLabel->setText("Directed by " + directors.join(", "));
                m_directorLabel->setVisible(true);
            } else {
                m_directorLabel->setVisible(false);
            }
        }

        // Cast
        if (credits.contains("cast") && credits["cast"].isArray() && m_castLayout) {
            QJsonArray cast = credits["cast"].toArray();
            int castToShow = qMin(10, cast.size());

            for (int i = 0; i < castToShow; i++) {
                QJsonObject member = cast[i].toObject();
                QString actor = member["name"].toString();
                QString character = member["character"].toString();
                QString profilePath = member["profile_path"].toString();

                QWidget *castItem = new QWidget();
                QHBoxLayout *castItemLayout = new QHBoxLayout(castItem);
                castItemLayout->setContentsMargins(0, 0, 0, 0);
                castItemLayout->setSpacing(15);

                // Profile photo
                QLabel *photoLabel = new QLabel();
                photoLabel->setObjectName("castPhoto");
                photoLabel->setFixedSize(50, 50);
                photoLabel->setStyleSheet(
                    "QLabel {"
                    "   background-color: " + AppTheme::SKELETON_BG + ";"
                    "   border-radius: 25px;"
                    "}"
                );

                if (!profilePath.isEmpty()) {
                    QString fullProfilePath = m_imageBaseUrl + m_profileSize + profilePath;
                    QNetworkRequest request;
                    request.setUrl(QUrl(fullProfilePath));
                    request.setRawHeader("X-Cast-Photo", "1");
                    request.setRawHeader("X-Cast-Index", QByteArray::number(i));
                    m_imageNetworkManager->get(request);
                }

                // Actor info
                QLabel *infoLabel = new QLabel(
                    QString("<b>%1</b><br>%2").arg(actor, character)
                );
                infoLabel->setStyleSheet(
                    "QLabel {"
                    "   color: " + AppTheme::TEXT_ON_DARK + ";"
                    "   font-size: 14px;"
                    "}"
                );
                infoLabel->setWordWrap(true);

                castItemLayout->addWidget(photoLabel);
                castItemLayout->addWidget(infoLabel, 1);

                m_castLayout->addWidget(castItem);
            }
        }
    }

    // Show action buttons
    if(m_playButton) m_playButton->setVisible(true);
    if(m_watchTrailerButton) m_watchTrailerButton->setVisible(true);
}

void MovieDetailWidget::fetchDetailData(int itemId, const QString& itemType)
{
    if(itemId <= 0 || itemType.isEmpty() || !m_detailNetworkManager) return;

    QString endpoint = QString("https://api.themoviedb.org/3/%1/%2?api_key=%3&append_to_response=credits")
                           .arg(itemType).arg(itemId).arg(m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    m_detailNetworkManager->get(request);
}

void MovieDetailWidget::fetchRecommendations(int itemId, const QString& itemType)
{
    if(itemId <= 0 || itemType.isEmpty() || !m_recommendationsNetworkManager) return;

    QString endpoint = QString("https://api.themoviedb.org/3/%1/%2/recommendations?api_key=%3")
                           .arg(itemType).arg(itemId).arg(m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    m_recommendationsNetworkManager->get(request);
}

void MovieDetailWidget::fetchVideos(int itemId, const QString& itemType)
{
    if(itemId <= 0 || itemType.isEmpty() || !m_videosNetworkManager) return;

    QString endpoint = QString("https://api.themoviedb.org/3/%1/%2/videos?api_key=%3")
                           .arg(itemType).arg(itemId).arg(m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    m_videosNetworkManager->get(request);
}

void MovieDetailWidget::fetchWatchProviders(int itemId, const QString& itemType)
{
    if(itemId <= 0 || itemType.isEmpty() || !m_providersNetworkManager) return;

    QString endpoint = QString("https://api.themoviedb.org/3/%1/%2/watch/providers?api_key=%3")
                           .arg(itemType).arg(itemId).arg(m_apiKey);

    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    m_providersNetworkManager->get(request);
}

void MovieDetailWidget::onAddToListClicked()
{
    if(m_currentItemId > 0 && !m_currentItemType.isEmpty() && !m_currentTitle.isEmpty()) {
        emit addToListRequested(m_currentItemId, m_currentItemType, m_currentTitle);

        // Visual feedback
        m_addToListButton->setText("✓ Added");
        m_addToListButton->setStyleSheet(
            "QPushButton {"
            "   background-color: " + AppTheme::PRIMARY_RED_ALPHA_15 + ";"
            "   color: " + AppTheme::PRIMARY_RED + ";"
            "   padding: 10px 25px;"
            "   border: 1px solid " + AppTheme::PRIMARY_RED + ";"
            "   border-radius: 6px;"
            "   min-width: 120px;"
            "   font-size: 14px;"
            "}"
        );

        // Reset after 2 seconds
        QTimer::singleShot(2000, this, [this]() {
            m_addToListButton->setText("+ Add to Watchlist");
            m_addToListButton->setStyleSheet(
                "QPushButton {"
                "   background-color: transparent;"
                "   color: " + AppTheme::TEXT_ON_DARK + ";"
                "   padding: 10px 25px;"
                "   border: 1px solid " + AppTheme::TEXT_ON_DARK_SECONDARY + ";"
                "   border-radius: 6px;"
                "   min-width: 120px;"
                "   font-size: 14px;"
                "}"
                "QPushButton:hover {"
                "   border-color: " + AppTheme::TEXT_ON_DARK + ";"
                "}"
            );
        });
    }
}

void MovieDetailWidget::onBackButtonClicked()
{
    emit goBackRequested();
}

void MovieDetailWidget::onPlayButtonClicked()
{
    if(m_currentItemId > 0 && !m_currentItemType.isEmpty()) {
        emit playRequested(m_currentItemId, m_currentItemType);
    }
}
