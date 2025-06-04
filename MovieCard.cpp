#include "MovieCard.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QNetworkReply>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>  // Add this include for mouse events

MovieCard::MovieCard(const QJsonObject &itemData,
                     const QString &imageBaseUrl,
                     const QString &posterSize,
                     QWidget *parent)
    : QWidget(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_imageBaseUrl(imageBaseUrl),
    m_posterSize(posterSize),
    m_itemData(itemData)  // Initialize the member variable
{
    if (m_imageBaseUrl.isEmpty() || m_posterSize.isEmpty()) {
        qWarning() << "MovieCard: Image base URL or poster size is empty. Poster loading might fail.";
        if(m_imageBaseUrl.isEmpty()) m_imageBaseUrl = "https://image.tmdb.org/t/p/";
        if(m_posterSize.isEmpty()) m_posterSize = "w500"; // Default fallback
    }
    setupUI(itemData);
}

// Add this mouse press event handler
void MovieCard::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);

    emit clicked(m_itemData);  // Now m_itemData exists
}

void MovieCard::setupUI(const QJsonObject &itemData) {
    this->setFixedSize(180, 300); // Slightly increased height for title/rating room

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // Card itself will have no internal margins for poster
    layout->setSpacing(6); // Spacing between poster, title, and rating

    // Poster image
    QLabel *posterLabel = new QLabel(this);
    posterLabel->setFixedSize(180, 240); // Poster area
    posterLabel->setAlignment(Qt::AlignCenter);
    posterLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px;")
                                   .arg(AppTheme::SKELETON_BG)); // Placeholder background

    // Determine title (movie vs. TV show)
    QString titleString;
    if (itemData.contains("title") && !itemData["title"].toString().isEmpty()) {
        titleString = itemData["title"].toString();
    } else if (itemData.contains("name") && !itemData["name"].toString().isEmpty()) {
        titleString = itemData["name"].toString(); // For TV shows
    } else {
        titleString = "N/A";
    }

    // Movie/TV title
    QLabel *title = new QLabel(titleString, this);
    title->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500; padding: 0 4px;") // Added padding
                             .arg(AppTheme::TEXT_ON_DARK));
    title->setWordWrap(true);
    title->setAlignment(Qt::AlignLeft | Qt::AlignTop); // Align text properly
    title->setMaximumWidth(172); // Max width considering padding
    // Give title label a minimum height to accommodate two lines if needed.
    QFontMetrics fm(title->font());
    title->setMinimumHeight(fm.lineSpacing() * 2); // Enough for two lines
    title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding); // Allow title to take available space


    // Rating
    double ratingValue = itemData["vote_average"].toDouble();
    QLabel *ratingLabel = new QLabel(QString::number(ratingValue, 'f', 1) + " ★", this);
    ratingLabel->setStyleSheet(QString("color: %1; font-size: 12px; padding: 0 4px;") // Added padding
                                   .arg(AppTheme::PRIMARY_RED));
    ratingLabel->setAlignment(Qt::AlignLeft);


    layout->addWidget(posterLabel);
    layout->addWidget(title);
    layout->addWidget(ratingLabel);
    layout->addStretch(1); // Push rating to bottom if there's extra space after title

    // Set object name for styling and identification
    this->setObjectName("MovieCardWidget");
    this->setStyleSheet(QString("#MovieCardWidget { background-color: transparent; border-radius: 8px; }" // Transparent base
                                "#MovieCardWidget:hover { background-color: %1; }") // Hover on the whole card
                            .arg(AppTheme::INTERACTIVE_HOVER_BG));


    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this); // Parent `this` for effect
    shadow->setBlurRadius(18); // Slightly more blur
    shadow->setColor(AppTheme::SHADOW_COLOR);
    shadow->setOffset(0, 6); // Slightly more offset
    this->setGraphicsEffect(shadow);

    // Load poster if available
    QString posterPath = itemData["poster_path"].toString();
    if (!posterPath.isEmpty()) {
        if (m_imageBaseUrl.endsWith("/")) m_imageBaseUrl.chop(1); // Ensure no double slash
        if (m_posterSize.startsWith("/")) m_posterSize = m_posterSize.mid(1);
        loadPoster(m_imageBaseUrl + "/" + m_posterSize + posterPath);
    } else {
        posterLabel->setText("No Image"); // Placeholder text
        posterLabel->setStyleSheet(QString("background-color: %1; border-radius: 8px; color: %2; qproperty-alignment: AlignCenter;")
                                       .arg(AppTheme::SKELETON_BG).arg(AppTheme::TEXT_ON_DARK));
    }
}

void MovieCard::loadPoster(const QString &fullPosterPath) {
    QUrl url(fullPosterPath);
    QNetworkRequest request(url);

    // Find the poster QLabel (assuming it's the first widget in the layout)
    QLabel *posterWidget = nullptr;
    if (this->layout() && this->layout()->count() > 0) {
        posterWidget = qobject_cast<QLabel*>(this->layout()->itemAt(0)->widget());
    }

    if (!posterWidget) {
        qDebug() << "MovieCard::loadPoster - Poster QLabel not found!";
        return;
    }

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, posterWidget, fullPosterPath]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());

            if (!pixmap.isNull()) {
                // Create rounded corners
                QPixmap rounded(posterWidget->size()); // Use label's size
                rounded.fill(Qt::transparent); // Transparent background for rounded corners

                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                QPainterPath path;
                path.addRoundedRect(rounded.rect(), 8, 8); // 8px radius
                painter.setClipPath(path);

                // Scale pixmap to fill the target size while keeping aspect ratio, then crop.
                // This ensures no letterboxing if aspect ratios differ slightly.
                QSize pixmapSize = pixmap.size();
                QSize targetSize = rounded.size();
                pixmapSize.scale(targetSize, Qt::KeepAspectRatioByExpanding);

                // Calculate offsets to center the scaled pixmap
                qreal x = (targetSize.width() - pixmapSize.width()) / 2.0;
                qreal y = (targetSize.height() - pixmapSize.height()) / 2.0;

                painter.drawPixmap(QRectF(x, y, pixmapSize.width(), pixmapSize.height()), pixmap, pixmap.rect());
                painter.end(); // Good practice to end painter explicitly

                posterWidget->setPixmap(rounded);
            } else {
                qDebug() << "MovieCard::loadPoster - Failed to load pixmap from data for URL:" << fullPosterPath;
                posterWidget->setText("Load Error");
            }
        } else {
            qDebug() << "MovieCard::loadPoster - Network error:" << reply->errorString() << "for URL:" << fullPosterPath;
            if (posterWidget) posterWidget->setText("Net Error");
        }
        reply->deleteLater();
    });
}
