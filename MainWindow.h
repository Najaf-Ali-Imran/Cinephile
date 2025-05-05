// --- START OF FILE MainWindow.h ---


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPoint>

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
class DashboardWidget;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void updateDbStatusIcon(bool connected);

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
    void onCategoryClicked(const QString &categoryName);

private:

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
    QList<QPushButton*> m_navButtons;
    QWidget* m_actorIconsContainer = nullptr;
    QList<QToolButton*> m_actorIconButtons;
    QList<QWidget*> m_categoryItemWidgets;


    QHBoxLayout *m_topBarLayout = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QLabel *m_dbStatusIconLabel = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QToolButton *m_userProfileButton = nullptr;
    QPushButton *m_minimizeButton = nullptr;
    QPushButton *m_maximizeButton = nullptr;
    QPushButton *m_closeButton = nullptr;


    bool m_dragging = false;
    QPoint m_dragStartPos;
    bool m_isMaximized = false;


    void setupUi();
    void setupSidebar();
    void setupTopBar();
    void setupMainContentArea();
    void setupActorIconsSection(QVBoxLayout* targetLayout);
    void applyStylesheets();
    void setupConnections();


    QPushButton* createNavigationButton(const QString &text, const QString &iconPath, int id);
    QLabel* createSectionTitle(const QString &text);
    QToolButton* createActorIconButton(const QString &actorName, const QString &iconPath);
    QWidget* createCategoryItemWidget(const QString &name, const QString &dotColor);

};

#endif // MAINWINDOW_H
// --- END OF FILE MainWindow.h ---
