// --- START OF FILE DashboardWidget.h ---

#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include "Theme.h"
#include "HomePage.h"
#include "MovieDetailWidget.h"

#include <QFrame>
#include <QList>
#include <QWidget>
#include <functional>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QPropertyAnimation;
class QPaintEvent;
class QLabel;
class QScrollArea;
QT_END_NAMESPACE

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
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override;

    using LayoutSetupFunction = void (DashboardWidget::*)(QWidget *);

    void showMovieDetails(int id, const QString &type);

public slots:
    void showHome();
    void showLibrary();
    void showCategories();
    void showFavorites();
    void showMore();
    void showProfile();
    void showSettings();
    void showSearchResults(const QString &query);

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

    QStackedWidget *m_contentStack = nullptr;

    QWidget *m_skeletonPage = nullptr;
    QWidget *m_libraryPage = nullptr;
    QWidget *m_categoriesPage = nullptr;
    QWidget *m_favoritesPage = nullptr;
    QWidget *m_profilePage = nullptr;
    QWidget *m_settingsPage = nullptr;
    QWidget *m_morePage = nullptr;

    HomePage* m_homePage = nullptr;
    MovieDetailWidget* m_movieDetailWidget = nullptr;  // Add this member

    QList<SkeletonElement *> m_skeletonElements;
    QList<SkeletonElement *> m_homeSkeletonElements;
    QList<SkeletonElement *> m_librarySkeletonElements;
    QList<SkeletonElement *> m_categoriesSkeletonElements;
    QList<SkeletonElement *> m_favoritesSkeletonElements;
    QList<SkeletonElement *> m_profileSkeletonElements;
    QList<SkeletonElement *> m_settingsSkeletonElements;
};

#endif // DASHBOARDWIDGET_H
// --- END OF FILE DashboardWidget.h ---
