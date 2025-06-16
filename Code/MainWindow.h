#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPoint>

class UserManager;
class LoginWidget;
class FirestoreService;
class DashboardWidget;
class GenreMoviesWidget;

QT_BEGIN_NAMESPACE
class QWidget;
class QVBoxLayout;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QToolButton;
class QLabel;
class QScrollArea;
class QMouseEvent;
class QButtonGroup;
class QEvent;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(LoginWidget *authService,
                        FirestoreService *firestoreService,
                        QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void updateDbStatusIcon(bool connected);
    void updateUserInfo();

    void onCategoryClicked(const QString &categoryName, int genreId);
signals:
    void aboutToLogout();
    void showGenreMoviesRequested(const QString &genreName, int genreId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onMinimizeClicked();
    void onMaximizeRestoreClicked();
    void onCloseClicked();
    void onSearchSubmitted();
    void onUserProfileClicked();
    void onSettingsClicked();
    void onNavigationSelected(int id);
    void onActorIconClicked(const QString &actorName);

    void onUserAuthenticatedChanged(bool isAuthenticated);
    void handleLogoutFromProfilePage();

    void fetchPopularActors();
    void processPopularActorsReply(QNetworkReply *reply);
    void fetchMovieGenres();
    void processMovieGenresReply(QNetworkReply *reply);

private:
    LoginWidget *m_loginWidgetService;
    FirestoreService *m_firestoreServiceInstance;
    QNetworkAccessManager *m_networkAccessManager;

    QWidget *m_centralWidget = nullptr;
    QHBoxLayout *m_mainLayout = nullptr;
    QWidget *m_sidebarWidget = nullptr;
    QWidget *m_rightContentContainer = nullptr;
    QVBoxLayout *m_rightContentLayout = nullptr;
    QWidget *m_topBarWidget = nullptr;
    DashboardWidget *m_dashboardWidget = nullptr;

    QVBoxLayout *m_sidebarLayout = nullptr;
    QLabel *m_logoLabel = nullptr;
    QScrollArea *m_sidebarScrollArea = nullptr;
    QWidget *m_sidebarScrollContentWidget = nullptr;
    QVBoxLayout *m_sidebarScrollContentLayout = nullptr;
    QButtonGroup *m_navButtonGroup = nullptr;
    QList<QPushButton *> m_navButtons;
    QWidget *m_actorIconsContainer = nullptr;
    QList<QToolButton *> m_actorIconButtons;
    QList<QWidget *> m_categoryItemWidgets;

    QMap<QString, int> m_genreNameToIdMap;

    QHBoxLayout *m_topBarLayout = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QLabel *m_dbStatusIconLabel = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QToolButton *m_userProfileButton = nullptr;
    QPushButton *m_minimizeButton = nullptr;
    QPushButton *m_maximizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;

    QLabel *m_userGreetingLabel = nullptr;

    bool m_dragging = false;
    QPoint m_dragStartPos;
    bool m_isMaximized = false;

    const QString TMDB_API_KEY = "b4c5ef5419f9f4ce8a627faa1e2be530";
    const QString TMDB_BASE_IMAGE_URL = "https://image.tmdb.org/t/p/";
    const QString TMDB_PROFILE_SIZE = "w185";
    const QString TMDB_POSTER_SIZE
        = "w342";

    void setupUi();
    void setupSidebar();
    void setupTopBar();
    void setupMainContentArea();
    void setupActorIconsSection(QVBoxLayout *targetLayout);
    void applyStylesheets();
    void setupConnections();

    QPushButton *createNavigationButton(const QString &text, const QString &iconPath, int id);
    QLabel *createSectionTitle(const QString &text);
    QToolButton *createActorIconButton(const QString &actorName,
                                       const QString &iconPath);
    QWidget *createCategoryItemWidget(const QString &name,
                                      const QString &dotColor,
                                      int genreId);
};

#endif
