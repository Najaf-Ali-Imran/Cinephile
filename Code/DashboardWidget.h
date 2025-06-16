#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QFrame>
#include <QJsonArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QWidget>
#include "CategoryFilterWidget.h"
#include "HomePage.h"
#include "MovieDetailWidget.h"
#include "Theme.h"
#include <functional>

class ProfilePageWidget;
class LoginWidget;
class FirestoreService;
class GenreMoviesWidget;
class MovieCard;
class WatchPage;
class LibraryPageWidget;
class RecommendationPageWidget;

class SkeletonElement : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(qreal shimmerPos READ shimmerPos WRITE setShimmerPos)

public:
    explicit SkeletonElement(int borderRadius = 8, QWidget *parent = nullptr);
    ~SkeletonElement() override;

    qreal shimmerPos() const { return m_shimmerPos; }
    void setShimmerPos(qreal pos)
    {
        m_shimmerPos = pos;
        update();
    }
    int m_borderRadius;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_shimmerPos = 0.0;
    QPropertyAnimation *m_animation = nullptr;
    static bool s_srandCalled;
};

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(LoginWidget *authService,
                             FirestoreService *firestoreService,
                             QNetworkAccessManager *networkManager,
                             QWidget *parent = nullptr);
    ~DashboardWidget() override;

    using LayoutSetupFunction = void (DashboardWidget::*)(QWidget *);
    void showMovieDetails(int id, const QString &type);
    void setServices(LoginWidget *authService, FirestoreService *firestoreService);

signals:
    void logoutSignal();
    void genreMovieClicked(int id, const QString &type);

public slots:
    void showHome();
    void showLibrary();
    void showCategories();
    void showWatch();
    void showMore();
    void showProfile();
    void showSettings();
    void showSearchResults(const QString &query);
    void showGenreMovies(const QString &genreName, int genreId);

private slots:
    void handleLogoutFromProfilePage();

private:
    void setupUi();
    QWidget *setupSkeletonPage(const QString &objectName,
                               QWidget *parent,
                               LayoutSetupFunction setupFunc);
    void setupHomeSkeletonLayout(QWidget *parentWidget);
    void setupLibrarySkeletonLayout(QWidget *parentWidget);
    void setupCategoriesSkeletonLayout(QWidget *parentWidget);
    void setupFavoritesSkeletonLayout(QWidget *parentWidget);
    void setupProfileSkeletonLayout(QWidget *parentWidget);
    void setupSettingsSkeletonLayout(QWidget *parentWidget);
    void addSkeletonElement(SkeletonElement *element, QWidget *pageWidget);
    QWidget *createPlaceholderPage(const QString &title);
    void displaySearchResults(const QJsonArray &results, const QString &query);

    QStackedWidget *m_stackedWidget = nullptr;
    LoginWidget *m_authService;
    FirestoreService *m_firestoreService;
    QNetworkAccessManager *m_networkAccessManager;
    QStackedWidget *m_contentStack = nullptr;

    QWidget *m_skeletonPage = nullptr;
    RecommendationPageWidget *m_recommendationPage = nullptr;
    QWidget *m_librarySkeletonPage = nullptr;
    QWidget *m_categoriesSkeletonPage = nullptr;
    QWidget *m_favoritesSkeletonPage = nullptr;
    QWidget *m_profileSkeletonPage = nullptr;
    QWidget *m_settingsSkeletonPage = nullptr;

    HomePage *m_homePage = nullptr;
    MovieDetailWidget *m_movieDetailWidget = nullptr;
    ProfilePageWidget *m_profilePageWidget = nullptr;
    QWidget *m_morePage = nullptr;
    GenreMoviesWidget *m_genreMoviesPage = nullptr;
    CategoryFilterWidget *m_categoryFilterPage = nullptr;
    WatchPage *m_watchPage = nullptr;
    LibraryPageWidget *m_libraryPage = nullptr;

    QList<SkeletonElement *> m_skeletonElements;
    QList<SkeletonElement *> m_homeSkeletonElements;
    QList<SkeletonElement *> m_librarySkeletonElements;
    QList<SkeletonElement *> m_categoriesSkeletonElements;
    QList<SkeletonElement *> m_favoritesSkeletonElements;
    QList<SkeletonElement *> m_profileSkeletonElements;
    QList<SkeletonElement *> m_settingsSkeletonElements;
};

#endif
