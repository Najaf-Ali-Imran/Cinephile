#ifndef MOVIEDETAILWIDGET_H
#define MOVIEDETAILWIDGET_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QMetaObject>
#include <QString>
#include <QWidget>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QNetworkReply;
class QStackedLayout;

class MovieDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MovieDetailWidget(const QString &apiKey, QWidget *parent = nullptr);
    ~MovieDetailWidget();

    void setImageBaseUrl(const QString &url) { m_imageBaseUrl = url; }
    void setBackdropSize(const QString &size) { m_backdropSize = size; }
    void setPosterSize(const QString &size) { m_posterSize = size; }

    void loadDetails(int itemId, const QString &itemType);

signals:
    void goBackRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onAddToListClicked();
    void updateAddToListButtonState();

private:
    void setupUI();
    void clearUI();
    void populateUI(const QJsonObject &detailData);
    QWidget *createHorizontalScrollerWithArrows(QHBoxLayout *contentLayout, int minHeight);

    void fetchDetails(int itemId, const QString &itemType);

    void populateCast(const QJsonArray &castArray);
    void populateVideos(const QJsonArray &videosArray);
    void populateRecommendations(const QJsonArray &recsArray);

    QStackedLayout *m_pageLayout;
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;

    QLabel *m_backdropLabel;

    QPushButton *m_addToListButton;

    QHBoxLayout *m_castLayout;
    QHBoxLayout *m_videosLayout;
    QHBoxLayout *m_recommendationsLayout;

    QNetworkAccessManager *m_networkManager;

    int m_currentItemId;
    QString m_currentItemType;
    QString m_apiKey;

    QString m_imageBaseUrl;
    QString m_backdropSize;
    QString m_posterSize;
    QString m_profileSize;

    QMetaObject::Connection m_parallaxConnection;
};

#endif
