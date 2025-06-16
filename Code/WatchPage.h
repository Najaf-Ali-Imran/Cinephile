#ifndef WATCHPAGE_H
#define WATCHPAGE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QWidget>

class QGridLayout;
class QScrollArea;
class QLineEdit;
class QPushButton;
class QLabel;
class QNetworkReply;
class MovieCard;
class QProgressBar;
class QFrame;
class QSvgWidget;
class QStackedLayout;

class WatchPage : public QWidget
{
    Q_OBJECT

public:
    explicit WatchPage(QNetworkAccessManager *networkManager, QWidget *parent = nullptr);
    ~WatchPage();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSearchSubmitted();
    void onMovieCardClicked(const QJsonObject &itemData);
    void processMovieReply(QNetworkReply *reply);

private:
    void setupUi();
    void fetchAndDisplayMovies(const QString &query);
    void clearGrid();
    void showStatusMessage(const QString &message, const QString &color);
    void showMovieGrid();
    void relayoutGrid();

    QStackedLayout *m_contentStack;
    QWidget *m_statusContainerWidget;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_movieGrid;
    QLabel *m_statusLabel;

    QLineEdit *m_searchBar;
    QPushButton *m_searchButton;
    QProgressBar *m_progressBar;

    QNetworkAccessManager *m_networkManager;
    QList<MovieCard *> m_movieCards;
    QString m_apiKey;
    int m_gridColumns;
};

#endif
