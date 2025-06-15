#include "DashboardWidget.h"
#include "CategoryFilterWidget.h"
#include "FirestoreService.h"
#include "GenreMoviesWidget.h"
#include "LibraryPageWidget.h"
#include "LoginWidget.h"
#include "MovieCard.h"
#include "ProfilePageWidget.h"
#include "Theme.h"
#include "WatchPage.h"
#include "RecommendationPageWidget.h"
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QScrollArea>
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

DashboardWidget::DashboardWidget(LoginWidget *authService,
                                 FirestoreService *firestoreService,
                                 QNetworkAccessManager *networkManager,
                                 QWidget *parent)
    : QWidget(parent)
    , m_authService(authService)
    , m_firestoreService(firestoreService)
    , m_networkAccessManager(networkManager)
    , m_homePage(nullptr)
    , m_movieDetailWidget(nullptr)
    , m_profilePageWidget(nullptr)
    , m_morePage(nullptr)
    , m_genreMoviesPage(nullptr)
    , m_categoryFilterPage(nullptr)
    , m_watchPage(nullptr)
    , m_libraryPage(nullptr)
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

    m_skeletonPage = setupSkeletonPage("homeSkeletonPage",
                                       this,
                                       &DashboardWidget::setupHomeSkeletonLayout);
    m_librarySkeletonPage = setupSkeletonPage("librarySkeletonPage",
                                              this,
                                              &DashboardWidget::setupLibrarySkeletonLayout);
    m_categoriesSkeletonPage = setupSkeletonPage("categoriesSkeletonPage",
                                                 this,
                                                 &DashboardWidget::setupCategoriesSkeletonLayout);
    m_favoritesSkeletonPage = setupSkeletonPage("favoritesSkeletonPage",
                                                this,
                                                &DashboardWidget::setupFavoritesSkeletonLayout);
    m_profileSkeletonPage = setupSkeletonPage("profileSkeletonPage",
                                              this,
                                              &DashboardWidget::setupProfileSkeletonLayout);
    m_settingsSkeletonPage = setupSkeletonPage("settingsSkeletonPage",
                                               this,
                                               &DashboardWidget::setupSettingsSkeletonLayout);

    m_morePage = createPlaceholderPage("More");

    m_contentStack->addWidget(m_skeletonPage);
    m_contentStack->addWidget(m_librarySkeletonPage);
    m_contentStack->addWidget(m_categoriesSkeletonPage);
    m_contentStack->addWidget(m_favoritesSkeletonPage);
    m_contentStack->addWidget(m_profileSkeletonPage);
    m_contentStack->addWidget(m_settingsSkeletonPage);
    m_contentStack->addWidget(m_morePage);

    mainLayout->addWidget(m_contentStack);
}

QWidget *DashboardWidget::setupSkeletonPage(const QString &objectName, QWidget *parent, LayoutSetupFunction setupFunc) {
    QScrollArea *scrollArea = new QScrollArea(parent);
    scrollArea->setObjectName(objectName + "ScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString("QScrollArea#%1ScrollArea { background-color: transparent; border: none; }").arg(objectName));
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *container = new QWidget();
    container->setObjectName(objectName);
    container->setStyleSheet(QString("QWidget#%1 { background-color: transparent; }").arg(objectName));
    (this->*setupFunc)(container);
    scrollArea->setWidget(container);
    return scrollArea;
}

void DashboardWidget::addSkeletonElement(SkeletonElement *element, QWidget *pageWidget) {
    if (!pageWidget) return;

    QString pageName = pageWidget->objectName();
    if (pageName == "homeSkeletonPage")
        m_homeSkeletonElements.append(element);
    else if (pageName == "librarySkeletonPage")
        m_librarySkeletonElements.append(element);
    else if (pageName == "categoriesSkeletonPage")
        m_categoriesSkeletonElements.append(element);
    else if (pageName == "favoritesSkeletonPage")
        m_favoritesSkeletonElements.append(element);
    else if (pageName == "profileSkeletonPage")
        m_profileSkeletonElements.append(element);
    else if (pageName == "settingsSkeletonPage")
        m_settingsSkeletonElements.append(element);
    m_skeletonElements.append(element);
}

void DashboardWidget::setupHomeSkeletonLayout(QWidget *parentWidget) {
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

void DashboardWidget::setupLibrarySkeletonLayout(QWidget *parentWidget) {
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

    QStringList sections = {"My Watchlist", "My Favorites", "Watched History"};
    for (const QString &title : sections) {
        SkeletonElement *sectionTitle = new SkeletonElement(6);
        sectionTitle->setFixedSize(200, 26);
        mainVLayout->addWidget(sectionTitle, 0, Qt::AlignLeft);
        addSkeletonElement(sectionTitle, parentWidget);

        QHBoxLayout *cardLayout = new QHBoxLayout();
        cardLayout->setSpacing(25);
        for (int i = 0; i < 5; ++i) {
            SkeletonElement *card = new SkeletonElement(8);
            card->setMinimumSize(160, 240);
            card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            cardLayout->addWidget(card);
            addSkeletonElement(card, parentWidget);
        }
        cardLayout->addStretch();
        mainVLayout->addLayout(cardLayout);
        mainVLayout->addSpacing(20);
    }
    mainVLayout->addStretch();
}

void DashboardWidget::setupCategoriesSkeletonLayout(QWidget *parentWidget) {
    qDeleteAll(m_categoriesSkeletonElements);
    m_categoriesSkeletonElements.clear();

    QGridLayout *layout = new QGridLayout(parentWidget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(25);

    for(int i=0; i<18; ++i) {
        SkeletonElement *card = new SkeletonElement(12);
        card->setMinimumHeight(120);
        layout->addWidget(card, i / 6, i % 6);
        addSkeletonElement(card, parentWidget);
    }

    for (int i=0; i<6; ++i)
        layout->setColumnStretch(i, 1);
    layout->setRowStretch(3, 1);
}

void DashboardWidget::setupFavoritesSkeletonLayout(QWidget *parentWidget) {
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
    for (int i = 0; i < 12; ++i) {
        SkeletonElement *card = new SkeletonElement(8);
        card->setMinimumSize(180, 140);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        gridLayout->addWidget(card, i / cols, i % cols);
        addSkeletonElement(card, parentWidget);
    }
    mainVLayout->addLayout(gridLayout);

    for (int i = 0; i < cols; ++i) {
        gridLayout->setColumnStretch(i, 1);
    }
    mainVLayout->addStretch();
}

void DashboardWidget::setupProfileSkeletonLayout(QWidget *parentWidget) {
    qDeleteAll(m_profileSkeletonElements);
    m_profileSkeletonElements.clear();

    QVBoxLayout *mainVLayout = new QVBoxLayout(parentWidget);
    mainVLayout->setContentsMargins(60, 50, 60, 50);
    mainVLayout->setSpacing(40);
    mainVLayout->setAlignment(Qt::AlignTop);

    QHBoxLayout *profileHeaderLayout = new QHBoxLayout();
    profileHeaderLayout->setSpacing(25);
    profileHeaderLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    SkeletonElement *avatar = new SkeletonElement(60);
    avatar->setFixedSize(120, 120);
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

    for (int j = 0; j < 2; ++j) {
        SkeletonElement *sectionTitle = new SkeletonElement(6);
        sectionTitle->setFixedSize(200, 26);
        mainVLayout->addWidget(sectionTitle, 0, Qt::AlignLeft);
        addSkeletonElement(sectionTitle, parentWidget);

        for (int i = 0; i < 2; ++i) {
            SkeletonElement *fieldLabel = new SkeletonElement(5);
            fieldLabel->setFixedSize(150, 20);
            mainVLayout->addWidget(fieldLabel, 0, Qt::AlignLeft);
            addSkeletonElement(fieldLabel, parentWidget);

            SkeletonElement *fieldInput = new SkeletonElement(6);
            fieldInput->setFixedHeight(38);
            fieldInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            mainVLayout->addWidget(fieldInput);
            addSkeletonElement(fieldInput, parentWidget);
            mainVLayout->addSpacing(5);
        }
        mainVLayout->addSpacing(15);
    }
    mainVLayout->addStretch();
}

void DashboardWidget::setupSettingsSkeletonLayout(QWidget *parentWidget) {
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

    for (int i = 0; i < 5; ++i) {
        QHBoxLayout *settingRow = new QHBoxLayout();
        settingRow->setSpacing(15);

        SkeletonElement *label = new SkeletonElement(6);
        label->setFixedSize(180, 24);
        settingRow->addWidget(label, 0, Qt::AlignLeft | Qt::AlignVCenter);
        addSkeletonElement(label, parentWidget);

        settingRow->addStretch();

        SkeletonElement *control = new SkeletonElement(6);
        if (i % 2 == 0) {
            control->setFixedSize(250, 30);
        } else {
            control->setFixedSize(60, 30);
            control->m_borderRadius = 15;
            control->setStyleSheet(QString("QWidget#skeletonElement { background-color: %1; border-radius: %2px; border: none; }").arg(AppTheme::SKELETON_BG).arg(control->m_borderRadius));
        }
        settingRow->addWidget(control, 0, Qt::AlignRight | Qt::AlignVCenter);
        addSkeletonElement(control, parentWidget);
        mainVLayout->addLayout(settingRow);
        mainVLayout->addSpacing(15);
    }
    mainVLayout->addStretch();
}

QWidget *DashboardWidget::createPlaceholderPage(const QString &title) {
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
    placeholderLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 24px; font-weight: bold; }").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    layout->addWidget(placeholderLabel);
    layout->addStretch();
    return page;
}

void DashboardWidget::setServices(LoginWidget *authService, FirestoreService *firestoreService)
{
    m_authService = authService;
    m_firestoreService = firestoreService;
}

void DashboardWidget::showHome()
{
    qDebug() << "DashboardWidget: Showing Home";
    if (!m_homePage) {
        qDebug() << "DashboardWidget: Creating HomePage instance.";
        m_homePage = new HomePage(this);
        m_contentStack->addWidget(m_homePage);
        connect(m_homePage, &HomePage::movieClicked, this, &DashboardWidget::showMovieDetails);
        connect(m_homePage, &HomePage::allDataLoaded, this, [this]() {
            qDebug() << "HomePage all data loaded - switching to it";
            if (m_contentStack->currentWidget() == m_skeletonPage
                || !m_contentStack->currentWidget()
                || m_contentStack->currentWidget()->objectName().contains("Skeleton")) {
                m_contentStack->setCurrentWidget(m_homePage);
            }
        });
    }

    if (m_homePage->isLoading()) {
        m_contentStack->setCurrentWidget(m_skeletonPage);
    } else {
        m_contentStack->setCurrentWidget(m_homePage);
    }
    m_homePage->loadData();
}

void DashboardWidget::showMovieDetails(int id, const QString &type)
{
    qDebug() << "DashboardWidget: Showing Movie Details for ID" << id << "Type:" << type;
    if (!m_movieDetailWidget) {
        m_movieDetailWidget = new MovieDetailWidget("b4c5ef5419f9f4ce8a627faa1e2be530", this);
        m_movieDetailWidget->setImageBaseUrl("https://image.tmdb.org/t/p/");
        m_movieDetailWidget->setBackdropSize("w1280");
        m_movieDetailWidget->setPosterSize("w500");

        connect(m_movieDetailWidget,
                &MovieDetailWidget::goBackRequested,
                this,
                &DashboardWidget::showHome);
        m_contentStack->addWidget(m_movieDetailWidget);
    }
    m_movieDetailWidget->loadDetails(id, type);
    m_contentStack->setCurrentWidget(m_movieDetailWidget);
}

void DashboardWidget::showLibrary()
{
    qDebug() << "DashboardWidget: Showing Library";

    if (!m_libraryPage) {
        m_libraryPage = new LibraryPageWidget(m_networkAccessManager, this);
        connect(m_libraryPage, &LibraryPageWidget::movieClicked, this, &DashboardWidget::showMovieDetails);
        m_contentStack->addWidget(m_libraryPage);
    }

    m_contentStack->setCurrentWidget(m_librarySkeletonPage);

    QTimer::singleShot(50, this, [this](){
        if(m_libraryPage) {
            m_libraryPage->loadLibraryData();
            m_contentStack->setCurrentWidget(m_libraryPage);
        }
    });
}

void DashboardWidget::showCategories()
{
    qDebug() << "DashboardWidget: Showing Categories";

    if (!m_categoryFilterPage) {
        m_contentStack->setCurrentWidget(m_categoriesSkeletonPage);

        QTimer::singleShot(100, this, [this](){
            m_categoryFilterPage = new CategoryFilterWidget(m_networkAccessManager, this);
            connect(m_categoryFilterPage,
                    &CategoryFilterWidget::movieClicked,
                    this,
                    &DashboardWidget::showMovieDetails);
            m_contentStack->addWidget(m_categoryFilterPage);
            m_contentStack->setCurrentWidget(m_categoryFilterPage);
        });

    } else {
        m_contentStack->setCurrentWidget(m_categoryFilterPage);
    }
}

void DashboardWidget::showWatch()
{
    qDebug() << "DashboardWidget: Showing Watch Page";
    if (!m_watchPage) {
        m_watchPage = new WatchPage(m_networkAccessManager, this);
        m_contentStack->addWidget(m_watchPage);
    }
    m_contentStack->setCurrentWidget(m_watchPage);
}

void DashboardWidget::showMore()
{
    qDebug() << "DashboardWidget: Showing Recommendations Page";
    if (!m_recommendationPage) {
        m_recommendationPage = new RecommendationPageWidget(m_networkAccessManager, this);
        connect(m_recommendationPage, &RecommendationPageWidget::movieClicked, this, &DashboardWidget::showMovieDetails);
        m_contentStack->addWidget(m_recommendationPage);
    }
    m_contentStack->setCurrentWidget(m_recommendationPage);
    m_recommendationPage->generateRecommendations();
}

void DashboardWidget::showProfile()
{
    qDebug() << "DashboardWidget: Showing Profile";
    if (!m_authService || !m_firestoreService) {
        qCritical() << "DashboardWidget::showProfile - Auth or Firestore service is not set!";
        if (m_profileSkeletonPage)
            m_contentStack->setCurrentWidget(m_profileSkeletonPage);
        return;
    }

    if (!m_profilePageWidget) {
        qDebug() << "DashboardWidget: Creating ProfilePageWidget instance.";

        m_profilePageWidget = new ProfilePageWidget(m_authService, m_firestoreService, this);
        m_profilePageWidget->setObjectName("profilePageWidget");
        connect(m_profilePageWidget,
                &ProfilePageWidget::logoutRequested,
                this,
                &DashboardWidget::handleLogoutFromProfilePage);
        m_contentStack->addWidget(m_profilePageWidget);
    }

    m_contentStack->setCurrentWidget(m_profilePageWidget);
    m_profilePageWidget->loadProfileData();
}

void DashboardWidget::showSettings()
{
    qDebug() << "DashboardWidget: Showing Settings (redirecting to Profile)";
    showProfile();
}

void DashboardWidget::handleLogoutFromProfilePage()
{
    emit logoutSignal();
}

void DashboardWidget::showSearchResults(const QString &query)
{
    qDebug() << "Dashboard: Showing Search Results for:" << query;

    QWidget* searchPage = m_contentStack->findChild<QWidget*>("searchresultsPageScrollArea");

    if (!searchPage) {
        QWidget *searchPageContent = new QWidget();
        searchPageContent->setObjectName("searchresultsPage");

        QScrollArea *searchScrollArea = new QScrollArea(this);
        searchScrollArea->setObjectName("searchresultsPageScrollArea");
        searchScrollArea->setWidgetResizable(true);
        searchScrollArea->setFrameShape(QFrame::NoFrame);
        searchScrollArea->setStyleSheet("background-color: transparent; border: none;");
        searchScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        searchScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        searchScrollArea->setWidget(searchPageContent);
        m_contentStack->addWidget(searchScrollArea);

        searchPage = searchScrollArea;
    }

    m_contentStack->setCurrentWidget(searchPage);

    QString apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";
    QString encodedQuery = QUrl::toPercentEncoding(query);
    QString urlStr = QString("https://api.themoviedb.org/3/search/multi?api_key=%1&query=%2")
                         .arg(apiKey, encodedQuery);

    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply *reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, query]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (jsonDoc.isObject()) {
                QJsonObject jsonObj = jsonDoc.object();
                QJsonArray results = jsonObj["results"].toArray();
                displaySearchResults(results, query);
            }
        } else {
            qWarning() << "Error fetching search results:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void DashboardWidget::displaySearchResults(const QJsonArray &results, const QString &query) {
    QScrollArea *scrollArea = m_contentStack->findChild<QScrollArea*>("searchresultsPageScrollArea");

    if (!scrollArea)
        return;

    QWidget* searchPageContent = scrollArea->widget();
    if (!searchPageContent)
        return;

    QLayout *existingLayout = searchPageContent->layout();
    if (existingLayout) {
        qDeleteAll(existingLayout->children());
        delete existingLayout;
    }

    QGridLayout *gridLayout = new QGridLayout(searchPageContent);
    gridLayout->setAlignment(Qt::AlignTop);
    gridLayout->setSpacing(20); gridLayout->setContentsMargins(40,25,40,25);
    int row = 0; int col = 0; const int cols = 5;

    for (const QJsonValue &value : results) {
        if (value.isObject()) {
            QJsonObject movieObj = value.toObject();
            MovieCard *movieCard = new MovieCard(movieObj, "https://image.tmdb.org/t/p/", "w342", this);

            connect(movieCard, &MovieCard::clicked, this, [this, movieObj]() {
                int movieId = movieObj["id"].toInt();
                QString movieType = movieObj["media_type"].toString();  // "movie", "tv", or "person"
                showMovieDetails(movieId, movieType);
            });
                gridLayout->addWidget(movieCard, row, col);
                col++; if (col >= cols) { col = 0; row++;
            }
        }
    }
}

void DashboardWidget::showGenreMovies(const QString &genreName, int genreId) {
    qDebug() << "Dashboard: Attempting to show movies for genre: " << genreName << " (ID: " << genreId << ")";
    if (!m_genreMoviesPage || m_genreMoviesPage->genreId() != genreId) {
        if (m_genreMoviesPage) {
            m_contentStack->removeWidget(m_genreMoviesPage);
            m_genreMoviesPage->deleteLater();
        }

        m_genreMoviesPage = new GenreMoviesWidget(genreId, genreName, m_networkAccessManager, this);

        connect(m_genreMoviesPage, &GenreMoviesWidget::movieClicked, this, &DashboardWidget::genreMovieClicked); m_contentStack->addWidget(m_genreMoviesPage); } m_contentStack->setCurrentWidget(m_genreMoviesPage); m_genreMoviesPage->fetchMovies();
}
