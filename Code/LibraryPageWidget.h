#ifndef LIBRARYPAGEWIDGET_H
#define LIBRARYPAGEWIDGET_H

#include <QWidget>
#include <QMap>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonValue>

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLabel;
class MovieCard;
class QNetworkReply;
class QPushButton;

class LibraryPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryPageWidget(QNetworkAccessManager *networkManager, QWidget *parent = nullptr);
    ~LibraryPageWidget() override;

public slots:
    void loadLibraryData();

signals:
    void movieClicked(int id, const QString &type);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void processMediaDetailsReply(QNetworkReply *reply, const QString &listType, QHBoxLayout *layout);
    void onCreateNewList();

private:
    void setupUi();
    void createHeaderSection();
    QWidget* createStatsWidget();
    void createCustomListsSection(QVBoxLayout *parentLayout);
    void updateLibraryStats(int watchlistCount, int favoritesCount, int historyCount);
    QWidget* createSection(const QString &title, QHBoxLayout *&cardLayout, const QString &iconName, bool isCustomList = false);
    QWidget* createHorizontalScrollerWithArrows(QHBoxLayout *&contentLayout, int minHeight);
    void fetchMediaDetails(const QJsonValue &mediaValue, const QString &listType, QHBoxLayout *layout);
    void clearLayout(QLayout *layout);
    QWidget* createMovieCardWidget(const QJsonObject &mediaObj, const QString &listName);
    void addPlaceholderMessage(QHBoxLayout* layout, const QString& message);

    QVBoxLayout *m_mainLayout;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;

    QHBoxLayout *m_watchlistLayout;
    QHBoxLayout *m_favoritesLayout;
    QHBoxLayout *m_historyLayout;

    QWidget* m_customListsContainer;
    QVBoxLayout *m_customListsLayout;

    QLabel* m_watchlistCountLabel;
    QLabel* m_favoritesCountLabel;
    QLabel* m_historyCountLabel;

    QMap<QString, QList<int>> m_loadedMediaIds;

    QLabel *m_statusLabel;

    QNetworkAccessManager *m_networkManager;
    const QString m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";
};

#endif
