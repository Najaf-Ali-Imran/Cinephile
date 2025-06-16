// --- GenreMoviesWidget.h ---

#ifndef GENREMOVIESWIDGET_H
#define GENREMOVIESWIDGET_H

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>

class QVBoxLayout;
class QScrollArea;
class QGridLayout;
class QLabel;
class MovieCard;

class GenreMoviesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GenreMoviesWidget(int genreId,
                               const QString &genreName,
                               QNetworkAccessManager *manager,
                               QWidget *parent = nullptr);
    ~GenreMoviesWidget() override = default;

    int genreId() const { return m_genreId; }

public slots:
    void fetchMovies();

signals:
    void movieClicked(int id, const QString &type);

private slots:
    void processMoviesReply(QNetworkReply *reply);

private:
    int m_genreId;
    QString m_genreName;
    QNetworkAccessManager *m_networkAccessManager;

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_moviesGridLayout;

    const QString TMDB_API_KEY = "b4c5ef5419f9f4ce8a627faa1e2be530";
    const QString TMDB_BASE_IMAGE_URL = "https://image.tmdb.org/t/p/";
    const QString TMDB_POSTER_SIZE = "w342";

    void clearMoviesLayout();
};

#endif
