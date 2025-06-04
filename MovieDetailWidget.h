#ifndef MOVIEDETAILWIDGET_H
#define MOVIEDETAILWIDGET_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QString>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QNetworkReply;

class MovieDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MovieDetailWidget(const QString &apiKey, QWidget *parent = nullptr);
    ~MovieDetailWidget();

    // Configuration setters
    void setImageBaseUrl(const QString &url) { m_imageBaseUrl = url; }
    void setBackdropSize(const QString &size) { m_backdropSize = size; }
    void setPosterSize(const QString &size) { m_posterSize = size; }
    void setProfileSize(const QString &size) { m_profileSize = size; }
    void setLogoSize(const QString &size) { m_logoSize = size; }

    void loadDetails(int itemId, const QString& itemType);

signals:
    void goBackRequested();
    void addToListRequested(int itemId, const QString& itemType, const QString& title);
    void playRequested(int itemId, const QString& itemType);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onBackButtonClicked();
    void onAddToListClicked();
    void onPlayButtonClicked();

private:
    void setupUI();
    void addSectionDivider(QLayout *layout);
    void safeDeleteLayoutItems(QLayout *layout);
    void populateUI(const QJsonObject &detailData);
    QPixmap createRoundedPixmap(const QPixmap &source);

    // Network request methods
    void fetchDetailData(int itemId, const QString& itemType);
    void fetchRecommendations(int itemId, const QString& itemType);
    void fetchVideos(int itemId, const QString& itemType);
    void fetchWatchProviders(int itemId, const QString& itemType);

    // Image loading methods
    void loadBackdropImage(const QString &fullBackdropPath);
    void loadPosterImage(const QString &fullPosterPath);

    // UI creation methods
    void createRecommendationItem(const QJsonObject &itemData);
    void createTagPill(const QString &text, QLayout *targetLayout);

    // Network response handlers
    void handleDetailNetworkResponse(QNetworkReply *reply);
    void handleImageNetworkResponse(QNetworkReply *reply);
    void handleRecommendationsResponse(QNetworkReply *reply);
    void handleVideosResponse(QNetworkReply *reply);
    void handleProvidersResponse(QNetworkReply *reply);

    // UI Elements
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;

    // Header
    QPushButton *m_backButton;

    // Backdrop
    QLabel *m_backdropLabel;

    // Poster
    QLabel *m_posterLabel;

    // Details
    QLabel *m_titleLabel;
    QLabel *m_taglineLabel;
    QLabel *m_overviewLabel;
    QLabel *m_metadataLabel;
    QLabel *m_releaseDateLabel;
    QLabel *m_ratingLabel;
    QLabel *m_directorLabel;
    QHBoxLayout *m_genresLayout;

    // Action buttons
    QPushButton *m_playButton;
    QPushButton *m_addToListButton;
    QPushButton *m_watchTrailerButton;

    // Sections
    QLabel *m_providersLabel;
    QHBoxLayout *m_providersLayout;
    QVBoxLayout *m_castLayout;
    QLabel *m_videosLabel;
    QHBoxLayout *m_videosLayout;
    QLabel *m_recommendationsLabel;
    QHBoxLayout *m_recommendationsLayout;

    // Network managers
    QNetworkAccessManager *m_detailNetworkManager;
    QNetworkAccessManager *m_imageNetworkManager;
    QNetworkAccessManager *m_recommendationsNetworkManager;
    QNetworkAccessManager *m_videosNetworkManager;
    QNetworkAccessManager *m_providersNetworkManager;

    // Current item info
    int m_currentItemId;
    QString m_currentItemType;
    QString m_currentTitle;
    QString m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";  // Added missing API key member

    // Image configuration
    QString m_imageBaseUrl;
    QString m_backdropSize;
    QString m_posterSize;
    QString m_profileSize;
    QString m_logoSize;

    // State
    bool m_loading;
};

#endif // MOVIEDETAILWIDGET_H
