#pragma once

#include <QWidget>
#include <QNetworkAccessManager>
#include <QScrollArea>
#include <QVBoxLayout> // Changed from QHBoxLayout for the main layout
#include <QJsonObject>
#include <QJsonArray>
#include "Theme.h" // Assuming Theme.h provides AppTheme::MAIN_WINDOW_BG, AppTheme::TEXT_ON_DARK etc.

// Forward declaration if MovieCard is in its own header
class MovieCard;

struct TmdbEndpoint {
    QString path;
    QString title;
    QString type; // "movie", "tv", "config", "hero"
};

class HomePage : public QWidget {
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
    void dataLoaded(); // Emitted after each section is loaded
    void allDataLoaded(); // Emitted when all initial sections are processed
    void loadFailed(const QString& error);
    void movieClicked(int id, const QString &type); // Example signal

public slots:
    void loadData(); // Renamed from loadTmdbData for clarity

private slots:
    void handleNetworkResponse(QNetworkReply *reply);
    void handleImageConfigResponse(QNetworkReply *reply);
    void onMovieCardClicked(const QJsonObject &itemData);

private:
    void setupUI();
    QWidget* createSection(const QString &title, const QString& sectionType); // General section creator
    void populateSection(QWidget* sectionWidget, const QJsonArray &items, const QString& itemType);
    void createHeroBanner(const QJsonObject &itemData); // For a single hero item
    void clearLayout(QLayout* layout);
    void fetchTmdbConfiguration();

    QNetworkAccessManager *m_networkManager;
    QNetworkAccessManager *m_imageConfigManager; // Separate manager for initial config
    QVBoxLayout *m_mainLayout;          // Overall vertical layout for the page
    QWidget *m_scrollContentWidget;     // Widget inside the main scroll area
    QVBoxLayout *m_scrollContentLayout; // Layout for m_scrollContentWidget, holds all sections

    QString m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530"; // Replace with your real API key
    QString m_tmdbImageBaseUrl;
    QString m_tmdbPosterSize = "w780"; // Default, can be updated from config
    QString m_tmdbBackdropSize = "w1280"; // Default, can be updated from config

    QList<TmdbEndpoint> m_endpoints;
    QMap<QString, QWidget*> m_sectionsMap; // To map endpoint path to its section widget for population

    bool m_loading = false;
    int m_pendingRequests = 0;
};
