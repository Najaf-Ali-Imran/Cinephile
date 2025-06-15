#include "genremovieswidget.h"
#include "MovieCard.h"
#include "Theme.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScrollArea>
#include <QVBoxLayout>

GenreMoviesWidget::GenreMoviesWidget(int genreId,
                                     const QString &genreName,
                                     QNetworkAccessManager *manager,
                                     QWidget *parent)
    : QWidget(parent)
    , m_genreId(genreId)
    , m_genreName(genreName)
    , m_networkAccessManager(manager)
{
    setStyleSheet(QString("background-color: %1;").arg(AppTheme::MAIN_WINDOW_BG));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(20);

    m_titleLabel = new QLabel(QString("Top Movies & TV Shows in %1").arg(m_genreName), this);
    m_titleLabel->setStyleSheet(
        QString("color: %1; font-size: 24px; font-weight: bold;").arg(AppTheme::TEXT_ON_DARK));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");

    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet("background: transparent;");
    m_moviesGridLayout = new QGridLayout(m_scrollContent);
    m_moviesGridLayout->setContentsMargins(0, 0, 0, 0);
    m_moviesGridLayout->setSpacing(20);

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea);
}

void GenreMoviesWidget::fetchMovies()
{
    clearMoviesLayout();

    QString url_str = QString("https://api.themoviedb.org/3/discover/"
                              "movie?api_key=%1&language=en-US&sort_by=vote_average.desc&vote_"
                              "count.gte=100&with_genres=%2")
                          .arg(TMDB_API_KEY)
                          .arg(m_genreId);

    QUrl q_url(url_str);
    QNetworkRequest request(q_url);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { processMoviesReply(reply); });

    QString url_tv_str = QString("https://api.themoviedb.org/3/discover/"
                                 "tv?api_key=%1&language=en-US&sort_by=vote_average.desc&vote_"
                                 "count.gte=100&with_genres=%2")
                             .arg(TMDB_API_KEY)
                             .arg(m_genreId);

    QUrl q_tv_url(url_tv_str);
    QNetworkRequest tv_request(q_tv_url);
    QNetworkReply *tv_reply = m_networkAccessManager->get(tv_request);
    connect(tv_reply, &QNetworkReply::finished, this, [this, tv_reply]() {
        processMoviesReply(tv_reply);
    });
}

void GenreMoviesWidget::processMoviesReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject()) {
            QJsonArray results = jsonDoc.object()["results"].toArray();

            int row = m_moviesGridLayout->rowCount();
            int col = 0;
            const int maxCols = 4;

            for (const QJsonValue &value : results) {
                if (value.isObject()) {
                    QJsonObject itemObj = value.toObject();
                    MovieCard *card = new MovieCard(itemObj,
                                                    TMDB_BASE_IMAGE_URL,
                                                    TMDB_POSTER_SIZE,
                                                    this);
                    connect(card, &MovieCard::clicked, this, [this](const QJsonObject &itemData) {
                        int id = itemData["id"].toInt();
                        QString type = itemData.contains("title") ? "movie" : "tv";
                        emit movieClicked(id, type);
                    });
                    m_moviesGridLayout->addWidget(card, row, col);
                    col++;
                    if (col >= maxCols) {
                        col = 0;
                        ++row;
                    }
                }
            }
        }
    } else {
        qWarning() << "Error fetching genre movies:" << reply->errorString();
    }
    reply->deleteLater();
}

void GenreMoviesWidget::clearMoviesLayout()
{
    QLayoutItem *child;
    while ((child = m_moviesGridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}
