#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include "Theme.h"

class MovieCard;

struct TmdbEndpoint
{
    QString path;
    QString title;
    QString type;
};

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage();

    bool isLoading() const { return m_loading; }
    QString getApiKey() const { return m_apiKey; }
    QString getImageBaseUrl() const { return m_tmdbImageBaseUrl; }
    QString getBackdropSize() const { return m_tmdbBackdropSize; }
    QString getPosterSize() const { return m_tmdbPosterSize; }

signals:
    void dataLoaded();
    void allDataLoaded();
    void loadFailed(const QString &error);
    void movieClicked(int id, const QString &type);

public slots:
    void loadData();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void handleNetworkResponse(QNetworkReply *reply);
    void handleImageConfigResponse(QNetworkReply *reply);
    void onMovieCardClicked(const QJsonObject &itemData);

private:
    void setupUI();
    QWidget *createSection(const QString &title, const QString &sectionType);
    void populateSection(QWidget *sectionWidget, const QJsonArray &items, const QString &itemType);
    void createHeroBanner(const QJsonObject &itemData);
    void tryFallbackImage(const QJsonObject &itemData, QLabel *imageLabel);
    void clearLayout(QLayout *layout);
    void fetchTmdbConfiguration();
    QWidget *createHorizontalScrollerWithArrows(QHBoxLayout *contentLayout);

    QNetworkAccessManager *m_networkManager;
    QNetworkAccessManager *m_imageConfigManager;
    QVBoxLayout *m_mainLayout;
    QWidget *m_scrollContentWidget;
    QVBoxLayout *m_scrollContentLayout;

    QString m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";
    QString m_tmdbImageBaseUrl;
    QString m_tmdbPosterSize = "w780";
    QString m_tmdbBackdropSize = "w1280";

    QList<TmdbEndpoint> m_endpoints;
    QMap<QString, QWidget *> m_sectionsMap;

    bool m_loading = false;
    int m_pendingRequests = 0;
};
