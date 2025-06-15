#include "MovieCard.h"
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QVBoxLayout>

MovieCard::MovieCard(const QJsonObject &itemData,
                     const QString &imageBaseUrl,
                     const QString &posterSize,
                     QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_imageBaseUrl(imageBaseUrl)
    , m_posterSize(posterSize)
    , m_itemData(itemData)
{
    if (m_imageBaseUrl.isEmpty())
        m_imageBaseUrl = "https://image.tmdb.org/t/p/";
    if (m_posterSize.isEmpty())
        m_posterSize = "w500";

    setupUI(itemData);
    this->setCursor(Qt::PointingHandCursor);
}

void MovieCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_itemData);
    }
    QWidget::mousePressEvent(event);
}

void MovieCard::setupUI(const QJsonObject &itemData)
{
    this->setFixedSize(200, 360);
    this->setObjectName("MovieCardWidget");
    this->setStyleSheet(
        QString("#MovieCardWidget { background-color: transparent; border-radius: 8px; }"
                "#MovieCardWidget:hover { background-color: %1; }")
            .arg(AppTheme::INTERACTIVE_HOVER_BG));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 5);
    layout->setSpacing(5);

    QLabel *posterLabel = new QLabel(this);
    posterLabel->setFixedSize(200, 295);
    posterLabel->setAlignment(Qt::AlignCenter);
    posterLabel->setStyleSheet(
        QString("background-color: %1; border-radius: 8px;").arg(AppTheme::SKELETON_BG));

    QString titleString;
    if (itemData.contains("title") && !itemData["title"].toString().isEmpty()) {
        titleString = itemData["title"].toString();
    } else if (itemData.contains("name") && !itemData["name"].toString().isEmpty()) {
        titleString = itemData["name"].toString();
    } else {
        titleString = "N/A";
    }

    QLabel *title = new QLabel(titleString, this);
    title->setObjectName("movieCardTitle");
    title->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 500; padding: 0 4px;")
                             .arg(AppTheme::TEXT_ON_DARK));
    title->setWordWrap(true);
    title->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    double ratingValue = itemData["vote_average"].toDouble();
    QLabel *ratingLabel = new QLabel(this);
    ratingLabel->setAlignment(Qt::AlignLeft);
    if (ratingValue > 0) {
        ratingLabel->setText(QString::number(ratingValue, 'f', 1) + " ★");
        ratingLabel->setStyleSheet(
            QString("color: %1; font-size: 13px; font-weight: bold; padding: 0 4px;")
                .arg(AppTheme::PRIMARY_RED));
    }

    layout->addWidget(posterLabel);
    layout->addWidget(title, 1);
    layout->addWidget(ratingLabel);

    QString posterPath = itemData["poster_path"].toString();
    if (!posterPath.isEmpty()) {
        loadPoster(m_imageBaseUrl + m_posterSize + posterPath);
    } else {
        posterLabel->setText("No Image");
        posterLabel->setStyleSheet(posterLabel->styleSheet()
                                   + QString(" color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    }
}

void MovieCard::loadPoster(const QString &fullPosterPath)
{
    QUrl url(fullPosterPath);
    QNetworkRequest request(url);
    QLabel *posterWidget = findChild<QLabel *>();

    if (!posterWidget) {
        return;
    }

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, posterWidget]() {
        if (!posterWidget) {
            reply->deleteLater();
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            if (pixmap.loadFromData(reply->readAll())) {
                QPixmap rounded(posterWidget->size());
                rounded.fill(Qt::transparent);

                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                QPainterPath path;
                path.addRoundedRect(rounded.rect(), 8, 8);
                painter.setClipPath(path);

                QSize pixmapSize = pixmap.size();
                pixmapSize.scale(rounded.size(), Qt::KeepAspectRatioByExpanding);

                qreal x = (rounded.width() - pixmapSize.width()) / 2.0;
                qreal y = (rounded.height() - pixmapSize.height()) / 2.0;

                painter.drawPixmap(QRectF(x, y, pixmapSize.width(), pixmapSize.height()),
                                   pixmap,
                                   pixmap.rect());

                posterWidget->setPixmap(rounded);
            } else {
                posterWidget->setText("Load Error");
            }
        } else {
            posterWidget->setText("Net Error");
        }
        reply->deleteLater();
    });
}
