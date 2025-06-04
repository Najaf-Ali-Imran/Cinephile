// --- START OF FILE DashboardWidget.cpp ---

#include "DashboardWidget.h"
#include "Theme.h"

#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QScrollArea> // Included
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <cstdlib>
#include <ctime>

bool SkeletonElement::s_srandCalled = false;

SkeletonElement::SkeletonElement(int borderRadius, QWidget *parent)
    : QFrame(parent)
    , m_borderRadius(borderRadius)
{
    if (!s_srandCalled) {
        srand(static_cast<unsigned int>(time(nullptr)));
        s_srandCalled = true;
    }

    setObjectName("skeletonElement");

    setStyleSheet(
        QString(
            "QWidget#skeletonElement { background-color: %1; border-radius: %2px; border: none; }")
            .arg(AppTheme::SKELETON_BG)
            .arg(m_borderRadius));

    m_animation = new QPropertyAnimation(this, "shimmerPos", this);
    m_animation->setDuration(1600);
    m_animation->setStartValue(-1.0);
    m_animation->setEndValue(2.0);
    m_animation->setLoopCount(-1);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);

    int randomDelay = rand() % 400;

    QTimer::singleShot(randomDelay, this, [this]() {
        if (m_animation) {
            m_animation->start();
        }
    });
}

SkeletonElement::~SkeletonElement() {}

void SkeletonElement::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    qreal shimmerWidthFraction = 0.7;
    qreal rectWidth = static_cast<qreal>(width());
    qreal shimmerPixelWidth = rectWidth * shimmerWidthFraction;
    qreal startOffset = m_shimmerPos * (rectWidth + shimmerPixelWidth) - shimmerPixelWidth;

    QLinearGradient shimmerGradient(startOffset, 0, startOffset + shimmerPixelWidth, 0);
    shimmerGradient.setColorAt(0.0, Qt::transparent);
    shimmerGradient.setColorAt(0.5, QColor(200, 200, 200, 50));
    shimmerGradient.setColorAt(1.0, Qt::transparent);

    QPainterPath path;
    path.addRoundedRect(rect(), m_borderRadius, m_borderRadius);
    painter.setClipPath(path);
    painter.fillRect(rect(), shimmerGradient);
}

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("dashboardWidget");
    this->setStyleSheet("QWidget#dashboardWidget { background-color: transparent; }");
    setupUi();
}

DashboardWidget::~DashboardWidget()
{
    qDeleteAll(m_skeletonElements);
    m_skeletonElements.clear();
}

void DashboardWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_contentStack = new QStackedWidget(this);
    m_contentStack->setObjectName("dashboardContentStack");
    m_contentStack->setStyleSheet(
        "QWidget#dashboardContentStack { background-color: transparent; }");

    m_skeletonPage = setupSkeletonPage("skeletonPage",
                                       this,
                                       &DashboardWidget::setupHomeSkeletonLayout);
    m_libraryPage = setupSkeletonPage("libraryPage",
                                      this,
                                      &DashboardWidget::setupLibrarySkeletonLayout);
    m_categoriesPage = setupSkeletonPage("categoriesPage",
                                         this,
                                         &DashboardWidget::setupCategoriesSkeletonLayout);
    m_favoritesPage = setupSkeletonPage("favoritesPage",
                                        this,
                                        &DashboardWidget::setupFavoritesSkeletonLayout);
    m_profilePage = setupSkeletonPage("profilePage",
                                      this,
                                      &DashboardWidget::setupProfileSkeletonLayout);
    m_settingsPage = setupSkeletonPage("settingsPage",
                                       this,
                                       &DashboardWidget::setupSettingsSkeletonLayout);
    m_morePage = createPlaceholderPage("More");

    m_contentStack->addWidget(m_skeletonPage);
    m_contentStack->addWidget(m_libraryPage);
    m_contentStack->addWidget(m_categoriesPage);
    m_contentStack->addWidget(m_favoritesPage);
    m_contentStack->addWidget(m_morePage);
    m_contentStack->addWidget(m_profilePage);
    m_contentStack->addWidget(m_settingsPage);

    mainLayout->addWidget(m_contentStack);
}

QWidget *DashboardWidget::setupSkeletonPage(const QString &objectName,
                                            QWidget *parent,
                                            LayoutSetupFunction setupFunc)
{
    // Use QScrollArea to ensure content can scroll if it exceeds view height
    QScrollArea *scrollArea = new QScrollArea(parent);
    scrollArea->setObjectName(objectName + "ScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        QString("QScrollArea#%1ScrollArea { background-color: transparent; border: none; }")
            .arg(objectName));
    // Add Scrollbar styling from MainWindow stylesheet if needed, or keep it simple
    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff); // Usually not needed horizontally
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); // Show only when needed

    QWidget *container = new QWidget(); // Content widget for the scroll area
    container->setObjectName(objectName);
    container->setStyleSheet(
        QString("QWidget#%1 { background-color: transparent; }").arg(objectName));
    (this->*setupFunc)(container); // Populate the container widget

    scrollArea->setWidget(container); // Put the container inside the scroll area
    return scrollArea;                // Return the scroll area to be added to the stack
}

void DashboardWidget::addSkeletonElement(SkeletonElement *element, QWidget *pageWidget)
{
    if (!pageWidget)
        return;
    QString pageName = pageWidget->objectName();
    // Remove "ScrollArea" suffix if present before checking
    pageName.remove("ScrollArea");

    if (pageName == "skeletonPage")
        m_homeSkeletonElements.append(element);
    else if (pageName == "libraryPage")
        m_librarySkeletonElements.append(element);
    else if (pageName == "categoriesPage")
        m_categoriesSkeletonElements.append(element);
    else if (pageName == "favoritesPage")
        m_favoritesSkeletonElements.append(element);
    else if (pageName == "profilePage")
        m_profileSkeletonElements.append(element);
    else if (pageName == "settingsPage")
        m_settingsSkeletonElements.append(element);

    m_skeletonElements.append(element); // Keep track of all elements for cleanup
}

void DashboardWidget::setupHomeSkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_homeSkeletonElements);
    m_homeSkeletonElements.clear();

    QGridLayout *layout = new QGridLayout(parentWidget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setHorizontalSpacing(30);
    layout->setVerticalSpacing(20);

    SkeletonElement *banner = new SkeletonElement(12);
    banner->setMinimumHeight(260);
    banner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(banner, 0, 0, 1, 4);
    addSkeletonElement(banner, parentWidget);

    SkeletonElement *title1 = new SkeletonElement(6);
    title1->setFixedSize(220, 26);
    layout->addWidget(title1, 1, 0, 1, 2, Qt::AlignVCenter | Qt::AlignLeft);
    addSkeletonElement(title1, parentWidget);

    for (int i = 0; i < 4; ++i) {
        SkeletonElement *card = new SkeletonElement(8);
        card->setMinimumSize(180, 140);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(card, 2, i);
        addSkeletonElement(card, parentWidget);
    }

    SkeletonElement *title2 = new SkeletonElement(6);
    title2->setFixedSize(200, 26);
    layout->addWidget(title2, 3, 0, 1, 2, Qt::AlignVCenter | Qt::AlignLeft);
    addSkeletonElement(title2, parentWidget);

    for (int i = 0; i < 4; ++i) {
        SkeletonElement *card = new SkeletonElement(8);
        card->setMinimumSize(180, 140);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(card, 4, i);
        addSkeletonElement(card, parentWidget);
    }

    for (int i = 0; i < layout->columnCount(); ++i) {
        layout->setColumnStretch(i, 1);
    }
    layout->setRowStretch(layout->rowCount(), 1);
}

void DashboardWidget::setupLibrarySkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_librarySkeletonElements);
    m_librarySkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(40, 40, 40, 40);
    mainVLayout->setSpacing(30);

    SkeletonElement *pageTitle = new SkeletonElement(8);
    pageTitle->setFixedSize(250, 35);
    mainVLayout->addWidget(pageTitle, 0, Qt::AlignLeft);
    addSkeletonElement(pageTitle, parentWidget);
    mainVLayout->addSpacing(10);

    QStringList sections = {"Recently Watched", "Liked Movies", "Recommendations"};
    for (const QString &title : sections) {
        SkeletonElement *sectionTitle = new SkeletonElement(6);
        sectionTitle->setFixedSize(200, 26);
        mainVLayout->addWidget(sectionTitle, 0, Qt::AlignLeft);
        addSkeletonElement(sectionTitle, parentWidget);

        // Horizontal layout for cards
        QHBoxLayout *cardLayout = new QHBoxLayout();
        cardLayout->setSpacing(25);
        for (int i = 0; i < 5; ++i) { // Show 5 cards horizontally per section
            SkeletonElement *card = new SkeletonElement(8);
            card->setMinimumSize(160, 120); // Slightly smaller cards for horizontal rows
            card->setSizePolicy(QSizePolicy::Fixed,
                                QSizePolicy::Fixed); // Fix size for horizontal layout
            cardLayout->addWidget(card);
            addSkeletonElement(card, parentWidget);
        }
        cardLayout->addStretch(); // Pushes cards to the left
        mainVLayout->addLayout(cardLayout);
        mainVLayout->addSpacing(20); // Space before next section
    }

    mainVLayout->addStretch(); // Pushes all sections up
}

void DashboardWidget::setupCategoriesSkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_categoriesSkeletonElements);
    m_categoriesSkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(40, 40, 40, 40);
    mainVLayout->setSpacing(30); // Spacing between category sections

    SkeletonElement *pageTitle = new SkeletonElement(8);
    pageTitle->setFixedSize(250, 35);
    mainVLayout->addWidget(pageTitle, 0, Qt::AlignLeft);
    addSkeletonElement(pageTitle, parentWidget);
    mainVLayout->addSpacing(10); // Space after main title

    // Example categories for skeleton
    QStringList categories = {"Action", "Comedy", "Drama", "Sci-Fi", "Horror", "Documentary"};
    for (const QString &title : categories) {
        // Category Title Skeleton
        SkeletonElement *catTitle = new SkeletonElement(6);
        catTitle->setFixedSize(180, 26); // Size for category title
        mainVLayout->addWidget(catTitle, 0, Qt::AlignLeft);
        addSkeletonElement(catTitle, parentWidget);
        mainVLayout->addSpacing(5); // Smaller space between title and cards

        // Horizontal layout for cards in this category
        QHBoxLayout *cardLayout = new QHBoxLayout();
        cardLayout->setSpacing(25);   // Spacing between cards
        for (int i = 0; i < 5; ++i) { // Show 5 cards horizontally per category
            SkeletonElement *card = new SkeletonElement(8);
            card->setMinimumSize(160, 120); // Card size
            card->setSizePolicy(QSizePolicy::Fixed,
                                QSizePolicy::Fixed); // Fixed size for horizontal layout
            cardLayout->addWidget(card);
            addSkeletonElement(card, parentWidget);
        }
        cardLayout->addStretch(); // Pushes cards to the left
        mainVLayout->addLayout(cardLayout);
        // Spacing after card row is handled by mainVLayout spacing (30)
    }

    mainVLayout->addStretch(); // Pushes all category sections up
}

void DashboardWidget::setupFavoritesSkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_favoritesSkeletonElements);
    m_favoritesSkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(40, 40, 40, 40);
    mainVLayout->setSpacing(30);

    SkeletonElement *pageTitle = new SkeletonElement(8);
    pageTitle->setFixedSize(250, 35);
    mainVLayout->addWidget(pageTitle, 0, Qt::AlignLeft);
    addSkeletonElement(pageTitle, parentWidget);
    mainVLayout->addSpacing(20);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(30);
    gridLayout->setVerticalSpacing(30);
    int cols = 4;
    for (int i = 0; i < 12; ++i) { // Show 12 favorites in a grid
        SkeletonElement *card = new SkeletonElement(8);
        card->setMinimumSize(180, 140); // Slightly larger cards for grid view
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); // Expandable in grid
        gridLayout->addWidget(card, i / cols, i % cols);
        addSkeletonElement(card, parentWidget);
    }
    mainVLayout->addLayout(gridLayout);

    for (int i = 0; i < cols; ++i) {
        gridLayout->setColumnStretch(i, 1);
    }
    mainVLayout->addStretch();
}

void DashboardWidget::setupProfileSkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_profileSkeletonElements);
    m_profileSkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(60, 50, 60, 50);
    mainVLayout->setSpacing(40);
    mainVLayout->setAlignment(Qt::AlignTop);

    QHBoxLayout *profileHeaderLayout = new QHBoxLayout();
    profileHeaderLayout->setSpacing(25);
    profileHeaderLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    SkeletonElement *avatar = new SkeletonElement(50); // Circular avatar
    avatar->setFixedSize(100, 100);
    profileHeaderLayout->addWidget(avatar);
    addSkeletonElement(avatar, parentWidget);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(10);
    SkeletonElement *nameLine = new SkeletonElement(6);
    nameLine->setFixedSize(280, 28);
    infoLayout->addWidget(nameLine);
    addSkeletonElement(nameLine, parentWidget);

    SkeletonElement *emailLine = new SkeletonElement(6);
    emailLine->setFixedSize(220, 22);
    infoLayout->addWidget(emailLine);
    addSkeletonElement(emailLine, parentWidget);

    profileHeaderLayout->addLayout(infoLayout);
    profileHeaderLayout->addStretch();
    mainVLayout->addLayout(profileHeaderLayout);

    QStringList sections = {"Library Snapshot", "Favorites Snapshot"};
    for (const QString &title : sections) {
        SkeletonElement *sectionTitle = new SkeletonElement(6);
        sectionTitle->setFixedSize(200, 26);
        mainVLayout->addWidget(sectionTitle, 0, Qt::AlignLeft);
        addSkeletonElement(sectionTitle, parentWidget);

        QHBoxLayout *cardLayout = new QHBoxLayout();
        cardLayout->setSpacing(20);
        for (int i = 0; i < 3; ++i) { // Show 3 snapshot cards
            SkeletonElement *card = new SkeletonElement(8);
            card->setMinimumSize(140, 100);
            card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // Fixed size
            cardLayout->addWidget(card);
            addSkeletonElement(card, parentWidget);
        }
        cardLayout->addStretch();
        mainVLayout->addLayout(cardLayout);
        // Spacing handled by mainVLayout spacing (40)
    }

    mainVLayout->addStretch();
}

void DashboardWidget::setupSettingsSkeletonLayout(QWidget *parentWidget)
{
    qDeleteAll(m_settingsSkeletonElements);
    m_settingsSkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(60, 50, 60, 50);
    mainVLayout->setSpacing(30);
    mainVLayout->setAlignment(Qt::AlignTop);

    SkeletonElement *pageTitle = new SkeletonElement(8);
    pageTitle->setFixedSize(250, 35);
    mainVLayout->addWidget(pageTitle, 0, Qt::AlignLeft);
    addSkeletonElement(pageTitle, parentWidget);
    mainVLayout->addSpacing(20);

    for (int i = 0; i < 5; ++i) { // 5 example setting rows
        QHBoxLayout *settingRow = new QHBoxLayout();
        settingRow->setSpacing(15);
        SkeletonElement *label = new SkeletonElement(6);
        label->setFixedSize(180, 24);
        settingRow->addWidget(label, 0, Qt::AlignLeft | Qt::AlignVCenter);
        addSkeletonElement(label, parentWidget);

        settingRow->addStretch();

        SkeletonElement *control = new SkeletonElement(6);
        if (i % 2 == 0) { // Example: Input field or dropdown
            control->setFixedSize(250, 30);
        } else { // Example: Toggle switch
            control->setFixedSize(60, 30);
            control->m_borderRadius = 15; // Make it pill-shaped
            control->setStyleSheet(QString("QWidget#skeletonElement { background-color: %1; "
                                           "border-radius: %2px; border: none; }")
                                       .arg(AppTheme::SKELETON_BG)
                                       .arg(control->m_borderRadius));
        }
        settingRow->addWidget(control, 0, Qt::AlignRight | Qt::AlignVCenter);
        addSkeletonElement(control, parentWidget);
        mainVLayout->addLayout(settingRow);
        mainVLayout->addSpacing(15); // Space between setting rows
    }

    mainVLayout->addStretch();
}

QWidget *DashboardWidget::createPlaceholderPage(const QString &title)
{
    // This function remains the same, creates a simple placeholder page
    QWidget *page = new QWidget();
    QString objName = title.toLower();
    objName.remove(QLatin1Char(' '));
    page->setObjectName(objName + "Page");

    page->setStyleSheet(QString("QWidget#%1Page { background-color: transparent; }").arg(objName));

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);

    QLabel *placeholderLabel = new QLabel(title + " Content Goes Here");
    placeholderLabel->setObjectName("placeholderContentLabel");
    placeholderLabel->setAlignment(Qt::AlignCenter);

    placeholderLabel->setStyleSheet(
        QString("QLabel { color: %1; font-size: 24px; font-weight: bold; }")
            .arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    layout->addWidget(placeholderLabel);
    layout->addStretch();

    return page;
}

// --- Public Slots ---
void DashboardWidget::showHome()
{
    qDebug() << "Home button clicked - showing home page";

    if (m_skeletonPage) {
        qDebug() << "Showing skeleton page temporarily";
        m_contentStack->setCurrentWidget(m_skeletonPage);
    }

    if (!m_homePage) {
        qDebug() << "Creating HomePage for the first time";
        m_homePage = new HomePage();
        m_contentStack->insertWidget(1, m_homePage);

        connect(m_homePage, &HomePage::movieClicked, this, &DashboardWidget::showMovieDetails);  // Add this

        connect(m_homePage, &HomePage::dataLoaded, this, [this]() {
            qDebug() << "HomePage data loaded - switching to it";
            m_contentStack->setCurrentWidget(m_homePage);
        });
    }

    qDebug() << "Loading TMDB data";
    m_homePage->loadData();
}

void DashboardWidget::showMovieDetails(int id, const QString &type) {
    if (!m_movieDetailWidget) {
        m_movieDetailWidget = new MovieDetailWidget(m_homePage->getApiKey());  // Assuming you add getApiKey() to HomePage
        m_movieDetailWidget->setImageBaseUrl(m_homePage->getImageBaseUrl());  // Add these getters to HomePage
        m_movieDetailWidget->setBackdropSize(m_homePage->getBackdropSize());
        m_movieDetailWidget->setPosterSize(m_homePage->getPosterSize());

        connect(m_movieDetailWidget, &MovieDetailWidget::goBackRequested, this, &DashboardWidget::showHome);

        m_contentStack->addWidget(m_movieDetailWidget);
    }

    m_movieDetailWidget->loadDetails(id, type);
    m_contentStack->setCurrentWidget(m_movieDetailWidget);
}



void DashboardWidget::showLibrary()
{
    if (m_libraryPage)
        m_contentStack->setCurrentWidget(m_libraryPage);
}
void DashboardWidget::showCategories()
{
    if (m_categoriesPage)
        m_contentStack->setCurrentWidget(m_categoriesPage);
}
void DashboardWidget::showFavorites()
{
    if (m_favoritesPage)
        m_contentStack->setCurrentWidget(m_favoritesPage);
}
void DashboardWidget::showMore()
{
    if (m_morePage)
        m_contentStack->setCurrentWidget(m_morePage);
}
void DashboardWidget::showProfile()
{
    if (m_profilePage)
        m_contentStack->setCurrentWidget(m_profilePage);
}
void DashboardWidget::showSettings()
{
    if (m_settingsPage)
        m_contentStack->setCurrentWidget(m_settingsPage);
}

void DashboardWidget::showSearchResults(const QString &query)
{
    qDebug() << "Dashboard: Showing Search Results for:" << query;

    QWidget *existingSearchPage = nullptr;
    for (int i = 0; i < m_contentStack->count(); ++i) {
        if (m_contentStack->widget(i)->objectName() == "searchresultsPage") {
            // Find the scroll area if it exists
            QScrollArea *scrollArea = qobject_cast<QScrollArea *>(m_contentStack->widget(i));
            if (scrollArea)
                existingSearchPage = scrollArea->widget(); // Get the widget inside
            else
                existingSearchPage = m_contentStack->widget(i); // Fallback if not a scroll area

            // Optionally update the label within the page content widget
            QLabel *label = existingSearchPage
                                ? existingSearchPage->findChild<QLabel *>("placeholderContentLabel")
                                : nullptr;
            if (label) {
                label->setText(QString("Search Results for '%1'").arg(query));
            }
            break; // Found the page (or its container)
        }
    }

    if (existingSearchPage) {
        // We want to show the container (ScrollArea) in the stack
        QWidget *containerWidget = nullptr;
        for (int i = 0; i < m_contentStack->count(); ++i) {
            if (m_contentStack->widget(i)->objectName() == "searchresultsPageScrollArea"
                || m_contentStack->widget(i)->objectName() == "searchresultsPage") {
                containerWidget = m_contentStack->widget(i);
                break;
            }
        }
        if (containerWidget)
            m_contentStack->setCurrentWidget(containerWidget);

    } else {
        // Create a new placeholder page inside a scroll area
        QWidget *searchPageContent = createPlaceholderPage(
            QString("Search Results for '%1'").arg(query));
        searchPageContent->setObjectName("searchresultsPage"); // Content widget name

        QScrollArea *searchScrollArea = new QScrollArea();              // Container scroll area
        searchScrollArea->setObjectName("searchresultsPageScrollArea"); // Name for the scroll area
        searchScrollArea->setWidgetResizable(true);
        searchScrollArea->setFrameShape(QFrame::NoFrame);
        searchScrollArea->setStyleSheet("background-color: transparent; border: none;");
        searchScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        searchScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        searchScrollArea->setWidget(searchPageContent); // Put content inside scroll area

        int index = m_contentStack->addWidget(searchScrollArea); // Add scroll area to stack
        m_contentStack->setCurrentIndex(index);
    }
}

// --- END OF FILE DashboardWidget.cpp ---
