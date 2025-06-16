#include "MainWindow.h"
#include "DashboardWidget.h"
#include "FirestoreService.h"
#include "LoginWidget.h"
#include "ProfilePageWidget.h"
#include "Theme.h"
#include "UserManager.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDebug>
#include <QFile>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

const int SIDEBAR_WIDTH = 240;
const int TOP_BAR_HEIGHT = 65;
const int WINDOW_CONTROL_SIZE = 30;
const int ACTOR_ICON_SIZE = 36;
const int CATEGORY_DOT_SIZE = 8;
const int DB_ICON_SIZE = 20;
const int TOP_BAR_PROFILE_PIC_SIZE = 38;

MainWindow::MainWindow(LoginWidget *authService, FirestoreService *firestoreService, QWidget *parent)
    : QMainWindow(parent)
    , m_loginWidgetService(authService)
    , m_firestoreServiceInstance(firestoreService)
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMinimumSize(1024, 720);
    resize(1280, 768);

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), screenGeometry));
    }

    setupUi();
    applyStylesheets();
    setupConnections();

    if (m_navButtonGroup && m_navButtonGroup->buttons().count() > 0) {
        m_navButtonGroup->button(0)->setChecked(true);
        onNavigationSelected(0);
    } else if (m_dashboardWidget) {
        m_dashboardWidget->showHome();
    }

    updateDbStatusIcon(false);
    updateUserInfo();
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("centralWidget");
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    setupSidebar();
    setupTopBar();
    setupMainContentArea();

    m_rightContentContainer = new QWidget();
    m_rightContentContainer->setObjectName("rightContentContainer");
    m_rightContentLayout = new QVBoxLayout(m_rightContentContainer);
    m_rightContentLayout->setSpacing(0);
    m_rightContentLayout->setContentsMargins(0, 0, 0, 0);
    m_rightContentLayout->addWidget(m_topBarWidget);
    m_rightContentLayout->addWidget(m_dashboardWidget, 1);

    m_mainLayout->addWidget(m_sidebarWidget);
    m_mainLayout->addWidget(m_rightContentContainer, 1);
}

void MainWindow::setupSidebar()
{
    m_sidebarWidget = new QWidget();
    m_sidebarWidget->setObjectName("sidebarWidget");
    m_sidebarWidget->setFixedWidth(SIDEBAR_WIDTH);

    m_sidebarLayout = new QVBoxLayout(m_sidebarWidget);
    m_sidebarLayout->setSpacing(0);
    m_sidebarLayout->setContentsMargins(0, 0, 0, 0);

    m_logoLabel = new QLabel();
    m_logoLabel->setObjectName("logoLabel");
    m_logoLabel->setFixedHeight(TOP_BAR_HEIGHT + 15);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    QPixmap logoPixmap(":/assets/icons/logoc.svg");
    if (!logoPixmap.isNull()) {
        m_logoLabel->setPixmap(
            logoPixmap.scaled(160, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_logoLabel->setText("CINEPHILE");
        qWarning() << "Sidebar Logo not found at ':/assets/icons/logo.svg'";
    }
    m_sidebarLayout->addWidget(m_logoLabel);

    m_sidebarScrollArea = new QScrollArea();
    m_sidebarScrollArea->setObjectName("sidebarScrollArea");
    m_sidebarScrollArea->setWidgetResizable(true);
    m_sidebarScrollArea->setFrameShape(QFrame::NoFrame);

    m_sidebarScrollContentWidget = new QWidget();
    m_sidebarScrollContentWidget->setObjectName("sidebarScrollContentWidget");

    m_sidebarScrollContentLayout = new QVBoxLayout(m_sidebarScrollContentWidget);
    m_sidebarScrollContentLayout->setSpacing(5);
    m_sidebarScrollContentLayout->setContentsMargins(15, 10, 15, 15);
    m_sidebarScrollContentLayout->setAlignment(Qt::AlignTop);

    m_navButtonGroup = new QButtonGroup(this);
    m_navButtonGroup->setExclusive(true);

    m_navButtons.append(createNavigationButton("Home", ":/assets/icons/home_icon.svg", 0));
    m_navButtons.append(createNavigationButton("Library", ":/assets/icons/library.svg", 1));
    m_navButtons.append(
        createNavigationButton("Categories", ":/assets/icons/categories_icon.svg", 2));
    m_navButtons.append(createNavigationButton("Watch", ":/assets/icons/watch.svg", 3));
    m_navButtons.append(createNavigationButton("CineAI", ":/assets/icons/more_icon.svg", 4));

    m_sidebarScrollContentLayout->addSpacing(10);

    setupActorIconsSection(m_sidebarScrollContentLayout);

    m_sidebarScrollContentLayout->addSpacing(10);


    m_sidebarScrollContentLayout->addWidget(createSectionTitle("Top Categories"));
    m_categoryItemWidgets.clear();

    fetchMovieGenres();

    m_sidebarScrollArea->setWidget(m_sidebarScrollContentWidget);
    m_sidebarLayout->addWidget(m_sidebarScrollArea, 1);
}

void MainWindow::setupTopBar()
{
    m_topBarWidget = new QWidget();
    m_topBarWidget->setObjectName("topBarWidget");
    m_topBarWidget->setFixedHeight(TOP_BAR_HEIGHT);

    m_topBarLayout = new QHBoxLayout(m_topBarWidget);
    m_topBarLayout->setSpacing(15);
    m_topBarLayout->setContentsMargins(25, 0, 10, 0);

    m_searchInput = new QLineEdit();
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText("Search movies, TV shows...");
    m_searchInput->setFixedHeight(38);
    m_searchInput->setMinimumWidth(200);
    m_searchInput->setMaximumWidth(450);

    QIcon searchIcon(":/assets/icons/search.svg");
    if (!searchIcon.isNull()) {
        QAction *searchAction = m_searchInput->addAction(searchIcon, QLineEdit::LeadingPosition);
        Q_UNUSED(searchAction); // To suppress unused variable warning
    } else {
        qWarning() << "Search icon not found at ':/assets/icons/search.svg'";
    }

    m_topBarLayout->addStretch();
    m_topBarLayout->addWidget(m_searchInput);
    m_topBarLayout->addStretch();

    m_dbStatusIconLabel = new QLabel();
    m_dbStatusIconLabel->setObjectName("dbStatusIcon");
    m_dbStatusIconLabel->setFixedSize(DB_ICON_SIZE + 10, DB_ICON_SIZE + 10);
    m_dbStatusIconLabel->setAlignment(Qt::AlignCenter);
    m_dbStatusIconLabel->setToolTip("Database Connection Status");
    m_topBarLayout->addWidget(m_dbStatusIconLabel);
    m_topBarLayout->addSpacing(5);

    m_settingsButton = new QToolButton();
    m_settingsButton->setObjectName("settingsButton");
    m_settingsButton->setFixedSize(TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    QIcon settingsIcon(":/assets/icons/settings.svg");
    if (!settingsIcon.isNull()) {
        m_settingsButton->setIcon(settingsIcon);
        m_settingsButton->setIconSize(QSize(18, 18));
    } else {
        m_settingsButton->setText("S");
        qWarning() << "Settings icon not found.";
    }
    m_settingsButton->setToolTip("Settings & Profile");
    m_topBarLayout->addWidget(m_settingsButton);

    m_userProfileButton = new QToolButton();
    m_userProfileButton->setObjectName("userProfileButton");
    m_userProfileButton->setFixedSize(TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
    m_userProfileButton->setIconSize(
        QSize(TOP_BAR_PROFILE_PIC_SIZE - 12,
              TOP_BAR_PROFILE_PIC_SIZE - 12)); // Icon smaller than button
    m_userProfileButton->setCursor(Qt::PointingHandCursor);
    m_userProfileButton->setToolTip("User Profile");

    QIcon userIcon(":/assets/icons/profile_icon.svg");

    if (!userIcon.isNull()) {
        m_userProfileButton->setIcon(userIcon);
    } else {
        m_userProfileButton->setText("U");
        qWarning() << "Default profile icon not found.";
    }
    m_topBarLayout->addWidget(m_userProfileButton);
    m_topBarLayout->addSpacing(10);

    m_minimizeButton = new QPushButton("—");
    m_minimizeButton->setObjectName("minimizeButton");
    m_minimizeButton->setFixedSize(WINDOW_CONTROL_SIZE, WINDOW_CONTROL_SIZE);
    m_minimizeButton->setCursor(Qt::PointingHandCursor);
    m_minimizeButton->setToolTip("Minimize");

    m_maximizeButton = new QPushButton("☐");
    m_maximizeButton->setObjectName("maximizeButton");
    m_maximizeButton->setFixedSize(WINDOW_CONTROL_SIZE, WINDOW_CONTROL_SIZE);
    m_maximizeButton->setCursor(Qt::PointingHandCursor);
    m_maximizeButton->setToolTip("Maximize");

    m_closeButton = new QPushButton("✕");
    m_closeButton->setObjectName("closeButton");
    m_closeButton->setFixedSize(WINDOW_CONTROL_SIZE, WINDOW_CONTROL_SIZE);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setToolTip("Close");

    m_topBarLayout->addWidget(m_minimizeButton);
    m_topBarLayout->addWidget(m_maximizeButton);
    m_topBarLayout->addWidget(m_closeButton);
}

void MainWindow::setupMainContentArea()
{
    if (!m_loginWidgetService || !m_firestoreServiceInstance || !m_networkAccessManager) {
        qCritical("MainWindow: Required service instance (LoginWidget, FirestoreService, or "
                  "NetworkAccessManager) is null! Profile page or network features may not work.");
    }

    m_dashboardWidget = new DashboardWidget(m_loginWidgetService,
                                            m_firestoreServiceInstance,
                                            m_networkAccessManager,
                                            this);
    m_dashboardWidget->setObjectName("dashboardContentWidget");
}

QPushButton *MainWindow::createNavigationButton(const QString &text, const QString &iconPath, int id)
{
    QPushButton *button = new QPushButton(text);
    QString objectName = "navButton_" + text;
    objectName.remove(QLatin1Char(' '));
    button->setObjectName(objectName);
    button->setProperty("class", "navigationButton");
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(44);
    QIcon icon(iconPath);
    if (!icon.isNull()) {
        button->setIcon(icon);
        button->setIconSize(QSize(18, 18));
    } else {
        qWarning() << "Navigation icon not found for" << text << "at" << iconPath;
    }
    m_sidebarScrollContentLayout->addWidget(button);
    m_navButtonGroup->addButton(button, id);
    return button;
}

QLabel *MainWindow::createSectionTitle(const QString &text)
{
    QLabel *label = new QLabel(text.toUpper());
    QString objectName = "sectionTitle_" + text;
    objectName.remove(QLatin1Char(' '));
    label->setObjectName(objectName);
    label->setProperty("class", "sectionTitle");
    return label;
}

void MainWindow::setupActorIconsSection(QVBoxLayout *targetLayout)
{
    targetLayout->addWidget(createSectionTitle("Top Actors"));
    m_actorIconsContainer = new QWidget();
    m_actorIconsContainer->setObjectName("actorIconsContainer");
    QHBoxLayout *actorLayout = new QHBoxLayout(m_actorIconsContainer);
    actorLayout->setContentsMargins(0, 5, 0, 5);
    actorLayout->setSpacing(8);
    actorLayout->setAlignment(Qt::AlignLeft);

    while (QLayoutItem *item = actorLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_actorIconButtons.clear();

    targetLayout->addWidget(m_actorIconsContainer);

    fetchPopularActors();
    actorLayout->addStretch();
}

QToolButton *MainWindow::createActorIconButton(const QString &actorName, const QString &iconPath)
{
    QToolButton *button = new QToolButton();
    QString baseName = actorName.simplified().replace(QLatin1Char(' '), QLatin1Char('_'));
    button->setObjectName("actorIcon_" + baseName);
    button->setProperty("class", "actorIconButton");
    button->setFixedSize(ACTOR_ICON_SIZE, ACTOR_ICON_SIZE);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(actorName);

    button->setText(actorName.isEmpty() ? "?" : QString(actorName.at(0)).toUpper());

    connect(button, &QToolButton::clicked, this, [this, actorName]() {
        onActorIconClicked(actorName);
    });

    if (!iconPath.isEmpty() && iconPath.startsWith("http")) {
        QUrl q_iconPathUrl(iconPath);
        QNetworkRequest request(q_iconPathUrl);
        QNetworkReply *reply = m_networkAccessManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, button, actorName, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pix;
                if (pix.loadFromData(reply->readAll())) {
                    QPixmap roundedPix(ACTOR_ICON_SIZE, ACTOR_ICON_SIZE);
                    roundedPix.fill(Qt::transparent);
                    QPainter painter(&roundedPix);
                    painter.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addEllipse(0, 0, ACTOR_ICON_SIZE, ACTOR_ICON_SIZE);
                    painter.setClipPath(path);
                    QPixmap scaled = pix.scaled(ACTOR_ICON_SIZE,
                                                ACTOR_ICON_SIZE,
                                                Qt::KeepAspectRatioByExpanding,
                                                Qt::SmoothTransformation);
                    int x = (ACTOR_ICON_SIZE - scaled.width()) / 2;
                    int y = (ACTOR_ICON_SIZE - scaled.height()) / 2;
                    painter.drawPixmap(x, y, scaled);

                    QIcon icon;
                    icon.addPixmap(roundedPix);
                    button->setIcon(icon);
                    button->setIconSize(QSize(ACTOR_ICON_SIZE - 4, ACTOR_ICON_SIZE - 4));
                    button->setText("");
                } else {
                    qWarning() << "Failed to load pixmap from data for " << actorName << " from "
                               << reply->url().toString();
                }
            } else {
                qWarning() << "Network error loading actor icon for " << actorName << ": "
                           << reply->errorString();
            }
            reply->deleteLater();
        });
    }
    else if (!iconPath.isEmpty() && iconPath.startsWith(":/") && QFile::exists(iconPath)) {
        QPixmap pix(iconPath);
        if (!pix.isNull()) {
            QPixmap roundedPix(ACTOR_ICON_SIZE, ACTOR_ICON_SIZE);
            roundedPix.fill(Qt::transparent);
            QPainter painter(&roundedPix);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, ACTOR_ICON_SIZE, ACTOR_ICON_SIZE);
            painter.setClipPath(path);
            QPixmap scaled = pix.scaled(ACTOR_ICON_SIZE,
                                        ACTOR_ICON_SIZE,
                                        Qt::KeepAspectRatioByExpanding,
                                        Qt::SmoothTransformation);
            int x = (ACTOR_ICON_SIZE - scaled.width()) / 2;
            int y = (ACTOR_ICON_SIZE - scaled.height()) / 2;
            painter.drawPixmap(x, y, scaled);

            QIcon icon;
            icon.addPixmap(roundedPix);
            button->setIcon(icon);
            button->setIconSize(QSize(ACTOR_ICON_SIZE - 4, ACTOR_ICON_SIZE - 4));
            button->setText("");
        } else {
            qWarning() << "Local SVG icon not valid for " << actorName << " at " << iconPath;
        }
    }
    else {

    }

    return button;
}

QWidget *MainWindow::createCategoryItemWidget(const QString &name,
                                              const QString &dotColor,
                                              int genreId)
{
    QWidget *itemWidget = new QWidget();
    QString baseName = name.simplified().replace(QLatin1Char(' '), QLatin1Char('_'));
    itemWidget->setObjectName("categoryItemWidget_" + baseName);
    itemWidget->setProperty("class", "categoryItemWidget");
    itemWidget->setCursor(Qt::PointingHandCursor);
    itemWidget->setSizePolicy(QSizePolicy::Expanding,
                              QSizePolicy::Preferred);

    QHBoxLayout *outerLayout = new QHBoxLayout(itemWidget);
    outerLayout->setContentsMargins(8, 5, 8, 5);
    outerLayout->setSpacing(10);

    QFrame *dot = new QFrame();
    dot->setObjectName("categoryColorDot");
    dot->setFixedSize(CATEGORY_DOT_SIZE, CATEGORY_DOT_SIZE);
    dot->setStyleSheet(QString("background-color: %1; border-radius: %2px; border: none;")
                           .arg(dotColor)
                           .arg(CATEGORY_DOT_SIZE / 2));

    QLabel *nameLabel = new QLabel(name);
    nameLabel->setObjectName("categoryNameLabel");

    outerLayout->addWidget(dot);
    outerLayout->addWidget(nameLabel);
    outerLayout->setAlignment(dot, Qt::AlignVCenter);
    outerLayout->setAlignment(nameLabel, Qt::AlignVCenter | Qt::AlignLeft);

    m_sidebarScrollContentLayout->addWidget(itemWidget);
    m_categoryItemWidgets.append(itemWidget);
    itemWidget->installEventFilter(this);
    itemWidget->setProperty("categoryName", name);
    itemWidget->setProperty("genreId", genreId);
    return itemWidget;
}

void MainWindow::applyStylesheets()
{
    QString sheet = QString(R"(
        QWidget#centralWidget { background-color: %1; }
        QWidget#rightContentContainer { background-color: transparent; border: none; }

        QWidget#sidebarWidget { background-color: %1;  }
        QLabel#logoLabel { color: %4; font-size: 18px; font-weight: bold; }
        QScrollArea#sidebarScrollArea { background: transparent; border: none; }
        QWidget#sidebarScrollContentWidget { background: transparent; }
        QScrollArea#sidebarScrollArea QScrollBar:vertical { background: transparent; border: none; width: 5px; margin: 0px; }
        QScrollArea#sidebarScrollArea QScrollBar::handle:vertical { background: transparent; min-height: 30px; border-radius: 2px; }
        QScrollArea#sidebarScrollArea QScrollBar::add-line:vertical, QScrollArea#sidebarScrollArea QScrollBar::sub-line:vertical { height: 0px; }
        QScrollArea#sidebarScrollArea QScrollBar::add-page:vertical, QScrollArea#sidebarScrollArea QScrollBar::sub-page:vertical { background: none; }
        QToolButton.actorIconButton { background-color: #333333; border: none; border-radius: 18px; color: #EEEBDD; font-weight: bold; font-size: 14px; }
        QToolButton.actorIconButton:hover { border: 1px solid #A0A0A0; }
        QWidget.categoryItemWidget { background: transparent; border-radius: 4px; padding: 3px 0; }
        QWidget.categoryItemWidget:hover { background: #2A2A2A; }
        QLabel#categoryNameLabel { color: #A0A0A0; font-size: 14px; background: transparent; padding-left: 0px; }

        QPushButton.navigationButton { background: transparent; border: none; color: %5; padding: 0 20px; text-align: left; font-size: 14px; border-radius: 6px; }
        QPushButton.navigationButton:hover { background: #2A2A2A; color: %5; }
        QPushButton.navigationButton:checked { background: %7; color: %5; font-weight: 600; }
        QPushButton.navigationButton:checked:hover { background: %7; color: %5; }
        QPushButton.navigationButton::icon { padding-right: 15px; }
        QScrollArea#sidebarScrollArea QScrollBar:horizontal {
                background: transparent;
                border: none;
                height: 5px;
                margin: 0px;
            }
            QScrollArea#sidebarScrollArea QScrollBar::handle:horizontal {
                background: transparent;
                min-width: 30px;
                border-radius: 2px;
            }
            QScrollArea#sidebarScrollArea QScrollBar::add-line:horizontal,
            QScrollArea#sidebarScrollArea QScrollBar::sub-line:horizontal {
                width: 0px;
            }
            QScrollArea#sidebarScrollArea QScrollBar::add-page:horizontal,
            QScrollArea#sidebarScrollArea QScrollBar::sub-page:horizontal {
                background: none;
            }

        QLabel.sectionTitle { color: %5; font-size: 10px; font-weight: 700; text-transform: uppercase; padding: 12px 0px 6px 8px; }


        QWidget#actorIconsContainer { background: transparent; }


        QWidget.categoryItemWidget:hover { background-color: transparent; transform: scale(1.05);}

        QWidget#topBarWidget { background-color: %1;  }
        QIcon#searchIcon { padding-left: 15px; }

        QLineEdit#searchInput { background: %9; border: 1px solid %3; border-radius: 19px; color: #EEEBDD; font-size: 13px; padding: 1px 15px 1px 5px;  selection-background-color: %6; selection-color: %4; }
        QLineEdit#searchInput:focus { border: 1px solid %16; }
        QLineEdit#searchInput::placeholder { color: %5; }

        QLabel#dbStatusIcon { background-color: transparent; border: none; border-radius: %19px; }

        QToolButton#settingsButton { background-color: transparent; border: none; border-radius: 19px; }
        QToolButton#settingsButton:hover { background-color: #2A2A2A; }
        QToolButton#settingsButton:pressed { background-color: #333333; }

        QToolButton#userProfileButton { background: %9; border: 1px solid %3; border-radius: 19px; padding: 0px; }
        QToolButton#userProfileButton:hover { background: %8; border-color: %5; }
        QToolButton#userProfileButton:pressed { background: %3; }


        QPushButton#minimizeButton, QPushButton#maximizeButton, QPushButton#closeButton { background: transparent; border: none; color: %5; font-size: 14px; font-weight: normal; border-radius: 4px; }
        QPushButton#minimizeButton:hover, QPushButton#maximizeButton:hover { background: %18; color: %4; }
        QPushButton#closeButton:hover { background: %14; color: #CE1212; }
        QPushButton#minimizeButton:pressed, QPushButton#maximizeButton:pressed { background: %3; }
        QPushButton#closeButton:pressed { background: %15; }


        QWidget#dashboardContentWidget { background-color: transparent; }
        QLabel#contentPlaceholderLabel { color: %5; font-size: 20px; }

    )")
                        .arg(AppTheme::MAIN_WINDOW_BG,
                             AppTheme::SIDEBAR_BG,
                             AppTheme::SKELETON_BG,
                             AppTheme::TEXT_ON_DARK,
                             AppTheme::TEXT_ON_DARK_SECONDARY,
                             AppTheme::PRIMARY_RED,
                             AppTheme::PRIMARY_RED_ALPHA_30,
                             AppTheme::INTERACTIVE_HOVER_BG,
                             AppTheme::INPUT_BG,
                             AppTheme::CONTENT_AREA_BG,
                             "UNUSED_11",
                             "UNUSED_12",
                             "UNUSED_13",
                             AppTheme::BUTTON_CLOSE_HOVER_BG,
                             AppTheme::BUTTON_CLOSE_PRESSED_BG,
                             AppTheme::INPUT_BORDER_FOCUS,
                             AppTheme::BUTTON_PRESSED_GENERAL_BG,
                             AppTheme::WINDOW_CONTROL_HOVER_BG,
                             QString::number((DB_ICON_SIZE + 15)
                                             / 2)); // Corrected radius for circle

    this->setStyleSheet(sheet);
}

void MainWindow::setupConnections()
{
    // Existing connections
    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(m_maximizeButton, &QPushButton::clicked, this, &MainWindow::onMaximizeRestoreClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchSubmitted);

    connect(m_userProfileButton, &QToolButton::clicked, this, &MainWindow::onUserProfileClicked);
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_navButtonGroup, &QButtonGroup::idClicked, this, &MainWindow::onNavigationSelected);

    connect(UserManager::instance(), &UserManager::userAuthenticatedChanged, this, &MainWindow::onUserAuthenticatedChanged);
    connect(UserManager::instance(), &UserManager::displayNameChanged, this, [this](const QString &) { updateUserInfo(); });
    connect(UserManager::instance(), &UserManager::profilePictureChanged, this, [this](const QString &) { updateUserInfo(); });
    connect(UserManager::instance(), &UserManager::emailChanged, this, [this](const QString &) { updateUserInfo(); });

    if (m_dashboardWidget) {
        connect(m_dashboardWidget, &DashboardWidget::logoutSignal, this, &MainWindow::handleLogoutFromProfilePage);
        connect(this, &MainWindow::showGenreMoviesRequested, m_dashboardWidget, &DashboardWidget::showGenreMovies);

        connect(m_dashboardWidget, &DashboardWidget::genreMovieClicked, this, [this](int id, const QString &type) {
            m_dashboardWidget->showMovieDetails(id, type);
        });
    }
}


void MainWindow::handleLogoutFromProfilePage()
{
    qDebug() << "Logout requested from Profile Page (via DashboardWidget).";
    UserManager::instance()->clearCurrentUser();
    emit aboutToLogout();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *widget = qobject_cast<QWidget *>(obj);

        if (widget && m_categoryItemWidgets.contains(widget)) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QString categoryName = widget->property("categoryName").toString();
                int genreId = widget->property("genreId").toInt();

                if (!categoryName.isEmpty()) {
                    onCategoryClicked(categoryName, genreId);
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint globalPos = event->globalPosition().toPoint();

        if (m_topBarWidget->rect().contains(m_topBarWidget->mapFromGlobal(globalPos))) {
            QWidget *child = m_topBarWidget->childAt(m_topBarWidget->mapFromGlobal(globalPos));
            bool onInteractiveChild = false;
            if (child) {
                onInteractiveChild = qobject_cast<QPushButton *>(child)
                                     || qobject_cast<QToolButton *>(child)
                                     || qobject_cast<QLineEdit *>(child)
                                     || (child->parentWidget()
                                         && qobject_cast<QLineEdit *>(child->parentWidget()))
                                     ||
                                     (child == m_dbStatusIconLabel);
            }

            if (!onInteractiveChild) {
                m_dragging = true;
                m_dragStartPos = globalPos - frameGeometry().topLeft();
                event->accept();
                return;
            }
        }
    }
    QMainWindow::mousePressEvent(event);
}
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QMainWindow::mouseReleaseEvent(event);
}
void MainWindow::onMinimizeClicked()
{
    showMinimized();
}
void MainWindow::onMaximizeRestoreClicked()
{
    if (m_isMaximized) {
        showNormal();
        m_maximizeButton->setText("☐");
        m_maximizeButton->setToolTip("Maximize");
    } else {
        showMaximized();
        m_maximizeButton->setText("❐");
        m_maximizeButton->setToolTip("Restore");
    }
    m_isMaximized = !m_isMaximized;
}
void MainWindow::onCloseClicked()
{
    QApplication::quit();
}

void MainWindow::onSearchSubmitted()
{
    QString searchText = m_searchInput->text().trimmed();
    if (!searchText.isEmpty() && m_dashboardWidget) {
        qDebug() << "Search submitted:" << searchText;
        m_dashboardWidget->showSearchResults(searchText);
        m_searchInput->clearFocus();
        if (m_navButtonGroup) {
            QAbstractButton *currentChecked = m_navButtonGroup->checkedButton();
            if (currentChecked) {
                m_navButtonGroup->setExclusive(false);
                currentChecked->setChecked(false);
                m_navButtonGroup->setExclusive(true);
            }
        }
    } else if (searchText.isEmpty()) {
        m_searchInput->clearFocus();
    }
}

void MainWindow::onSettingsClicked()
{
    if (m_dashboardWidget) {
        m_dashboardWidget->showProfile();
        if (m_navButtonGroup) {
            QAbstractButton *currentChecked = m_navButtonGroup->checkedButton();
            if (currentChecked) {
                m_navButtonGroup->setExclusive(false);
                currentChecked->setChecked(false);
                m_navButtonGroup->setExclusive(true);
            }
        }
    }
}
void MainWindow::onUserProfileClicked()
{
    if (m_dashboardWidget) {
        m_dashboardWidget->showProfile();
        if (m_navButtonGroup) {
            QAbstractButton *currentChecked = m_navButtonGroup->checkedButton();
            if (currentChecked) {
                m_navButtonGroup->setExclusive(false);
                currentChecked->setChecked(false);
                m_navButtonGroup->setExclusive(true);
            }
        }
    }
}

void MainWindow::onNavigationSelected(int id)
{
    if (!m_dashboardWidget) {
        return;
    }
    if (!m_navButtonGroup || !m_navButtonGroup->button(id)) {
        return;
    }

    QString selectedNavText = m_navButtonGroup->button(id)->text();
    qDebug() << "Navigation selected:" << selectedNavText << "(ID: " << id << ")";
    switch (id) {
    case 0:
        m_dashboardWidget->showHome();
        break;
    case 1:
        m_dashboardWidget->showLibrary();
        break;
    case 2:
        m_dashboardWidget
            ->showCategories();
        break;
    case 3:
        m_dashboardWidget->showWatch();
        break;
    case 4: m_dashboardWidget->showMore(); break;
    default:
        m_dashboardWidget->showHome();
        break;
    }
}

void MainWindow::onActorIconClicked(const QString &actorName)
{

}

void MainWindow::onCategoryClicked(const QString &categoryName, int genreId)
{
    qDebug() << "Category (Item) clicked:" << categoryName << "(ID: " << genreId << ")";
    emit showGenreMoviesRequested(categoryName, genreId);

    if (m_navButtonGroup) {
        QAbstractButton *currentChecked = m_navButtonGroup->checkedButton();
        if (currentChecked) {
            m_navButtonGroup->setExclusive(false);
            currentChecked->setChecked(false);
            m_navButtonGroup->setExclusive(true);
        }
    }
}

void MainWindow::updateDbStatusIcon(bool connected)
{
    if (!m_dbStatusIconLabel)
        return;
    QString iconPath = connected ? ":/assets/icons/db_connected.svg"
                                 : ":/assets/icons/db_disconnected.svg";
    QPixmap statusPixmap(iconPath);
    if (!statusPixmap.isNull()) {
        m_dbStatusIconLabel->setPixmap(statusPixmap.scaled(DB_ICON_SIZE,
                                                           DB_ICON_SIZE,
                                                           Qt::KeepAspectRatio,
                                                           Qt::SmoothTransformation));
        m_dbStatusIconLabel->setToolTip(connected ? "Online" : "Offline");
    } else {
        qWarning() << "Database status icon not found:" << iconPath;
        m_dbStatusIconLabel->setText(connected ? "✓" : "✕"); // Fallback text
        m_dbStatusIconLabel->setStyleSheet(
            QString("QLabel#dbStatusIcon { color: %1; font-weight: bold; font-size: 12px; }")
                .arg(connected ? "lightgreen" : AppTheme::PRIMARY_RED));
    }
}

void MainWindow::updateUserInfo()
{
    if (!m_userProfileButton)
        return;

    if (UserManager::instance()->isAuthenticated()) {
        QString displayName = UserManager::instance()->getDisplayName();
        if (displayName.isEmpty()) {
            displayName = UserManager::instance()->getEmail().split("@").first();
        }

        QString picPath = UserManager::instance()->getProfilePictureUrl();
        if (!picPath.isEmpty() && picPath.startsWith("http")) {
            QUrl picUrl(picPath);
            QNetworkRequest picRequest(picUrl);
            QNetworkReply *picReply = m_networkAccessManager->get(picRequest);
            connect(picReply, &QNetworkReply::finished, this, [this, picReply]() {
                if (picReply->error() == QNetworkReply::NoError) {
                    QPixmap userPixmap;
                    if (userPixmap.loadFromData(picReply->readAll())) {
                        QPixmap roundedPixmap(TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
                        roundedPixmap.fill(Qt::transparent);
                        QPainter painter(&roundedPixmap);
                        painter.setRenderHint(QPainter::Antialiasing);
                        QPainterPath path;
                        path.addEllipse(0, 0, TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
                        painter.setClipPath(path);
                        QPixmap scaled = userPixmap.scaled(TOP_BAR_PROFILE_PIC_SIZE,
                                                           TOP_BAR_PROFILE_PIC_SIZE,
                                                           Qt::KeepAspectRatioByExpanding,
                                                           Qt::SmoothTransformation);
                        int x = (TOP_BAR_PROFILE_PIC_SIZE - scaled.width()) / 2;
                        int y = (TOP_BAR_PROFILE_PIC_SIZE - scaled.height()) / 2;
                        painter.drawPixmap(x, y, scaled);
                        m_userProfileButton->setIcon(QIcon(roundedPixmap));
                    } else {
                        qWarning() << "User profile picture failed to load from data. URL:"
                                   << picReply->url().toString();
                        QIcon defaultUserIcon(":/assets/icons/profile_icon.svg");
                        if (!defaultUserIcon.isNull())
                            m_userProfileButton->setIcon(defaultUserIcon);
                        else
                            m_userProfileButton->setText("U");
                    }
                } else {
                    qWarning() << "Network error loading user profile picture:"
                               << picReply->errorString() << "URL:" << picReply->url().toString();
                    QIcon defaultUserIcon(":/assets/icons/profile_icon.svg");
                    if (!defaultUserIcon.isNull())
                        m_userProfileButton->setIcon(defaultUserIcon);
                    else
                        m_userProfileButton->setText("U");
                }
                picReply->deleteLater();
            });
        } else if (!picPath.isEmpty()) {
            QPixmap userPixmap(picPath);
            if (!userPixmap.isNull()) {
                QPixmap roundedPixmap(TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
                roundedPixmap.fill(Qt::transparent);
                QPainter painter(&roundedPixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, TOP_BAR_PROFILE_PIC_SIZE, TOP_BAR_PROFILE_PIC_SIZE);
                painter.setClipPath(path);
                QPixmap scaled = userPixmap.scaled(TOP_BAR_PROFILE_PIC_SIZE,
                                                   TOP_BAR_PROFILE_PIC_SIZE,
                                                   Qt::KeepAspectRatioByExpanding,
                                                   Qt::SmoothTransformation);
                int x = (TOP_BAR_PROFILE_PIC_SIZE - scaled.width()) / 2;
                int y = (TOP_BAR_PROFILE_PIC_SIZE - scaled.height()) / 2;
                painter.drawPixmap(x, y, scaled);
                m_userProfileButton->setIcon(QIcon(roundedPixmap));
            } else {
                QIcon defaultUserIcon(":/assets/icons/profile_icon.svg");
                if (!defaultUserIcon.isNull())
                    m_userProfileButton->setIcon(defaultUserIcon);
                else
                    m_userProfileButton->setText("U");
                qWarning() << "User profile picture not found or failed to load. Path:" << picPath;
            }
        } else {

            QIcon defaultUserIcon(":/assets/icons/profile_icon.svg");
            if (!defaultUserIcon.isNull())
                m_userProfileButton->setIcon(defaultUserIcon);
            else
                m_userProfileButton->setText("U");
            if (!picPath.isEmpty())
                qWarning() << "User profile picture not found or failed to load. Path:" << picPath;
        }

    } else {
        QIcon defaultUserIcon(":/assets/icons/profile_icon.svg");
        if (!defaultUserIcon.isNull())
            m_userProfileButton->setIcon(defaultUserIcon);
        else
            m_userProfileButton->setText("G");
    }
}

void MainWindow::fetchPopularActors()
{
    QString url_str
        = QString("https://api.themoviedb.org/3/person/popular?api_key=%1&language=en-US&page=1")
              .arg(TMDB_API_KEY);

    QUrl q_url(url_str);
    QNetworkRequest request(q_url);
    QNetworkReply *reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processPopularActorsReply(reply);
    });
}

void MainWindow::processPopularActorsReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QJsonArray results = jsonObj["results"].toArray();

            QHBoxLayout *actorLayout = qobject_cast<QHBoxLayout *>(m_actorIconsContainer->layout());
            if (!actorLayout) {
                qWarning() << "Actor icons container layout is not a QHBoxLayout!";
                reply->deleteLater();
                return;
            }

            while (QLayoutItem *item = actorLayout->takeAt(0)) {
                if (item->spacerItem()) {
                    delete item;
                } else if (QWidget *widget = item->widget()) {
                    widget->deleteLater();
                    delete item;
                } else {
                    delete item;
                }
            }
            m_actorIconButtons.clear();

            int actorsCount = 0;
            for (const QJsonValue &value : std::as_const(results)) {
                if (actorsCount >= 5) {
                    break;
                }
                if (value.isObject()) {
                    QJsonObject actorObj = value.toObject();
                    QString actorName = actorObj["name"].toString();
                    QString profilePath = actorObj["profile_path"].toString();

                    if (!actorName.isEmpty() && !profilePath.isEmpty()) {
                        QString imageUrl = QString("%1%2%3")
                        .arg(TMDB_BASE_IMAGE_URL)
                            .arg(TMDB_PROFILE_SIZE)
                            .arg(profilePath);

                        QToolButton *button = createActorIconButton(actorName, imageUrl);
                        m_actorIconButtons.append(button);
                        actorLayout->addWidget(button);
                        actorsCount++;
                    } else if (!actorName.isEmpty()) {
                        QToolButton *button = createActorIconButton(actorName, "");
                        m_actorIconButtons.append(button);
                        actorLayout->addWidget(button);
                        actorsCount++;
                    }
                }
            }
            actorLayout->addStretch();
        } else {
            qWarning() << "Failed to parse JSON documen.";
        }
    } else {
        qWarning() << "Error fetching popular actors: " << reply->errorString();
    }
    reply->deleteLater();
}

void MainWindow::fetchMovieGenres()
{
    QString url_str = QString(
                          "https://api.themoviedb.org/3/genre/movie/list?api_key=%1&language=en-US")
                          .arg(TMDB_API_KEY);

    qInfo() << "Fetching movie genres from URL: " << url_str;

    QUrl q_url(url_str);
    QNetworkRequest request(q_url);
    QNetworkReply *reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        processMovieGenresReply(reply);
    });
}

void MainWindow::processMovieGenresReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QJsonArray genres = jsonObj["genres"].toArray();

            for (int i = m_categoryItemWidgets.size() - 1; i >= 0; --i) {
                QWidget *widget = m_categoryItemWidgets.at(i);
                if (widget) {
                    m_sidebarScrollContentLayout->removeWidget(widget);
                    widget->deleteLater();
                }
            }
            m_categoryItemWidgets.clear();
            m_genreNameToIdMap.clear();

            QList<QString> categoryColors = {
                "#4287f5",
                "#32a852",
                "#f5a142",
                "#a842f5",
                "#f5429e",
                "#42f5ef",
                "#E0E0E0",
                "#f55d42",
                "#964B00",
                "#808000",
                "#DAA520",
                "#B0C4DE"
            };
            int colorIndex = 0;
            int count = 0;
            for (const QJsonValue &value : std::as_const(genres)) {
                if (count >= 5)
                    break;
                if (value.isObject()) {
                    QJsonObject genreObj = value.toObject();
                    int id = genreObj["id"].toInt();
                    QString name = genreObj["name"].toString();

                    if (!name.isEmpty()) {
                        QString color = categoryColors.at(colorIndex % categoryColors.size());
                        createCategoryItemWidget(name, color, id);
                        m_genreNameToIdMap.insert(name, id);
                        colorIndex++;
                        count++;
                    }
                }
            }
            qInfo() << "Successfully fetched and displayed " << m_categoryItemWidgets.size()
                    << " movie genres.";
        } else {
            qWarning() << "Failed to parse JSON document (genres) or it's not an object.";
        }
    } else {
        qWarning() << "Error fetching movie genres: " << reply->errorString();
    }
    reply->deleteLater();
}

void MainWindow::onUserAuthenticatedChanged(bool isAuthenticated)
{
    updateUserInfo();
    updateDbStatusIcon(isAuthenticated);

    m_settingsButton->setEnabled(isAuthenticated);
    m_userProfileButton->setEnabled(isAuthenticated);

    if (!isAuthenticated) {
        if (m_dashboardWidget) {
            m_dashboardWidget->showHome();
        }
        if (m_navButtonGroup && m_navButtonGroup->buttons().count() > 0) {
            m_navButtonGroup->button(0)->setChecked(true); // Default to Home
        }
    }
}
