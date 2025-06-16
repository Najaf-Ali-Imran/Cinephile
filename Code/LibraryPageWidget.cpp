#include "LibraryPageWidget.h"
#include "MovieCard.h"
#include "Theme.h"
#include "UserManager.h"

#include <QDebug>
#include <QEvent>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

LibraryPageWidget::LibraryPageWidget(QNetworkAccessManager *networkManager, QWidget *parent)
    : QWidget(parent)
    , m_networkManager(networkManager)
    , m_watchlistLayout(nullptr)
    , m_favoritesLayout(nullptr)
    , m_historyLayout(nullptr)
    , m_customListsContainer(nullptr)
    , m_customListsLayout(nullptr)
    , m_watchlistCountLabel(nullptr)
    , m_favoritesCountLabel(nullptr)
    , m_historyCountLabel(nullptr)
{
    setObjectName("libraryPageWidget");
    setupUi();
    connect(UserManager::instance(), &UserManager::profileDataChanged, this, &LibraryPageWidget::loadLibraryData);
}

LibraryPageWidget::~LibraryPageWidget()
{
}

void LibraryPageWidget::setupUi()
{
    this->setStyleSheet(QString("QWidget#libraryPageWidget { background-color: #000000; }"));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    createHeaderSection();

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(QString(
                                    "QScrollArea { background: transparent; border: none; }"
                                    "QScrollBar:vertical { background: %1; width: 8px; border-radius: 4px; }"
                                    "QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; }"
                                    "QScrollBar::handle:vertical:hover { background: %3; }"
                                    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; }"
                                    ).arg("rgba(255,255,255,0.1)", "rgba(255,255,255,0.3)", "rgba(255,255,255,0.5)"));

    m_scrollContent = new QWidget();
    m_scrollContent->setObjectName("libraryScrollContent");
    m_scrollContent->setStyleSheet("QWidget#libraryScrollContent { background-color: transparent; }");

    QVBoxLayout *scrollContentLayout = new QVBoxLayout(m_scrollContent);
    scrollContentLayout->setContentsMargins(40, 30, 40, 40);
    scrollContentLayout->setSpacing(35);
    scrollContentLayout->setAlignment(Qt::AlignTop);

    scrollContentLayout->addWidget(createSection("My Watchlist", m_watchlistLayout, "bookmark"));
    scrollContentLayout->addWidget(createSection("My Favorites", m_favoritesLayout, "heart"));
    scrollContentLayout->addWidget(createSection("Watch History", m_historyLayout, "clock"));

    createCustomListsSection(scrollContentLayout);

    m_statusLabel = new QLabel("Loading your library...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QString(
                                     "QLabel { color: %1; font-size: 16px; font-weight: 400; "
                                     "background: rgba(255,255,255,0.05); border-radius: 12px; "
                                     "padding: 24px; margin: 20px; }"
                                     ).arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    scrollContentLayout->addWidget(m_statusLabel);
    m_statusLabel->hide();

    scrollContentLayout->addStretch();

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea);
}

void LibraryPageWidget::createHeaderSection()
{
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(120);
    headerWidget->setStyleSheet("QWidget { background: #000000; border: none; }");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(40, 20, 40, 20);

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon(":/assets/icons/library").pixmap(40, 40));
    iconLabel->setStyleSheet("QLabel { background: transparent; }");

    QLabel *titleLabel = new QLabel("My Library");
    titleLabel->setObjectName("libraryTitleLabel");
    titleLabel->setStyleSheet(QString(
                                  "QLabel { color: %1; font-size: 32px; font-weight: 700; "
                                  "background: transparent; margin-left: 15px; }"
                                  ).arg(AppTheme::TEXT_ON_DARK));

    QLabel *subtitleLabel = new QLabel("Your personal collection of movies and shows");
    subtitleLabel->setStyleSheet(QString(
                                     "QLabel { color: %1; font-size: 16px; font-weight: 400; "
                                     "background: transparent; margin-left: 15px; margin-top: 5px; }"
                                     ).arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addWidget(iconLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    QWidget *statsWidget = createStatsWidget();
    headerLayout->addWidget(statsWidget);

    m_mainLayout->addWidget(headerWidget);
}

QWidget* LibraryPageWidget::createStatsWidget()
{
    QWidget *statsContainer = new QWidget();
    statsContainer->setStyleSheet("QWidget { background: transparent; border: none; }");
    statsContainer->setMinimumHeight(110);

    QHBoxLayout *statsLayout = new QHBoxLayout(statsContainer);
    statsLayout->setContentsMargins(20, 15, 20, 15);
    statsLayout->setSpacing(40);

    auto createStatItem = [&](const QString &icon, const QString &label) -> QWidget* {
        QWidget *item = new QWidget();
        QVBoxLayout *itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(8);

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(QString(":/assets/icons/%1").arg(icon)).pixmap(24, 24));
        iconLabel->setAlignment(Qt::AlignCenter);

        QLabel* countLabel = new QLabel("0");
        countLabel->setAlignment(Qt::AlignCenter);
        countLabel->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 600;").arg(AppTheme::TEXT_ON_DARK));

        QLabel *labelLabel = new QLabel(label);
        labelLabel->setAlignment(Qt::AlignCenter);
        labelLabel->setStyleSheet(QString("color: %1; font-size: 14px;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));

        itemLayout->addWidget(iconLabel);
        itemLayout->addWidget(countLabel);
        itemLayout->addWidget(labelLabel);

        if (label == "Watchlist") m_watchlistCountLabel = countLabel;
        else if (label == "Favorites") m_favoritesCountLabel = countLabel;
        else if (label == "Watched") m_historyCountLabel = countLabel;

        return item;
    };

    statsLayout->addWidget(createStatItem("bookmark", "Watchlist"));
    statsLayout->addWidget(createStatItem("heart", "Favorites"));
    statsLayout->addWidget(createStatItem("clock", "Watched"));

    return statsContainer;
}

void LibraryPageWidget::createCustomListsSection(QVBoxLayout *parentLayout)
{
    QWidget *customListsHeader = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(customListsHeader);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *customListIcon = new QLabel();
    customListIcon->setPixmap(QIcon(":/assets/icons/list").pixmap(24, 24));

    QLabel *customListTitle = new QLabel("Custom Lists");
    customListTitle->setStyleSheet(QString(
                                       "color: %1; font-size: 24px; font-weight: 600; margin-left: 10px;"
                                       ).arg(AppTheme::TEXT_ON_DARK));

    QPushButton *createListButton = new QPushButton(" Create New List");
    createListButton->setIcon(QIcon(":/assets/icons/plus"));
    createListButton->setCursor(Qt::PointingHandCursor);
    createListButton->setStyleSheet(QString(
                                        "QPushButton { "
                                        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2); "
                                        "color: white; border: none; padding: 10px 20px; border-radius: 8px; "
                                        "font-size: 14px; font-weight: 500; }"
                                        "QPushButton:hover { "
                                        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %3, stop:1 %4); "
                                        "transform: translateY(-1px); }"
                                        "QPushButton:pressed { transform: translateY(0px); }"
                                        ).arg(AppTheme::PRIMARY_RED, "#d01040", "#ff1744", "#e91e63"));

    createListButton->setIconSize(QSize(16, 16));
    createListButton->setFixedHeight(42);
    connect(createListButton, &QPushButton::clicked, this, &LibraryPageWidget::onCreateNewList);

    headerLayout->addWidget(customListIcon);
    headerLayout->addWidget(customListTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(createListButton);

    parentLayout->addWidget(customListsHeader);

    m_customListsContainer = new QWidget();
    m_customListsLayout = new QVBoxLayout(m_customListsContainer);
    m_customListsLayout->setContentsMargins(0, 0, 0, 0);
    m_customListsLayout->setSpacing(30);
    parentLayout->addWidget(m_customListsContainer);
}

QWidget* LibraryPageWidget::createSection(const QString &title, QHBoxLayout *&cardLayout, const QString &iconName, bool isCustomList)
{
    QWidget *sectionWidget = new QWidget();
    sectionWidget->setStyleSheet("QWidget { background: #000000; border: none; }");

    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionWidget);
    sectionLayout->setContentsMargins(25, 20, 25, 20);
    sectionLayout->setSpacing(18);

    QWidget* titleBar = new QWidget();
    QHBoxLayout* titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *iconLabel = new QLabel();
    if (!iconName.isEmpty()) {
        iconLabel->setPixmap(QIcon(QString(":/assets/icons/%1").arg(iconName)).pixmap(22, 22));
        iconLabel->setStyleSheet("QLabel { background: transparent; }");
    }

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setObjectName("librarySectionTitle");
    titleLabel->setStyleSheet(QString(
                                  "color: %1; font-size: 20px; font-weight: 600; margin-left: 8px;"
                                  ).arg(AppTheme::TEXT_ON_DARK));

    titleBarLayout->addWidget(iconLabel);
    titleBarLayout->addWidget(titleLabel);
    titleBarLayout->addStretch();

    if (isCustomList) {
        auto createButton = [&](const QString& text, const QString& iconPath, const QString& color = AppTheme::TEXT_ON_DARK_SECONDARY) {
            QPushButton* btn = new QPushButton(QString(" %1").arg(text));
            btn->setCursor(Qt::PointingHandCursor);
            btn->setIcon(QIcon(iconPath));
            btn->setIconSize(QSize(14, 14));
            btn->setStyleSheet(QString(
                                   "QPushButton { color: %1; border: 1px solid rgba(255,255,255,0.1); "
                                   "background: rgba(255,255,255,0.05); font-size: 13px; padding: 6px 12px; "
                                   "border-radius: 6px; margin-left: 8px; } "
                                   "QPushButton:hover { color: %2; background: rgba(255,255,255,0.1); "
                                   "border-color: %2; }"
                                   ).arg(color, AppTheme::PRIMARY_RED));
            return btn;
        };

        QPushButton* renameButton = createButton("Rename", ":/assets/icons/edit");
        QPushButton* deleteButton = createButton("Delete", ":/assets/icons/trash", "#ff4757");
        titleBarLayout->addWidget(renameButton);
        titleBarLayout->addWidget(deleteButton);

        connect(renameButton, &QPushButton::clicked, this, [this, title](){
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename List", "Enter new name for the list:", QLineEdit::Normal, title, &ok);
            if (ok && !newName.isEmpty() && newName != title) {
                UserManager::instance()->renameCustomList(title, newName);
            }
        });

        connect(deleteButton, &QPushButton::clicked, this, [this, title](){
            QMessageBox msgBox;
            msgBox.setWindowTitle("Delete List");
            msgBox.setText(QString("Are you sure you want to delete the list '%1'?").arg(title));
            msgBox.setInformativeText("This action cannot be undone.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);

            if (msgBox.exec() == QMessageBox::Yes) {
                UserManager::instance()->deleteCustomList(title, {});
            }
        });
    }

    sectionLayout->addWidget(titleBar);

    QWidget *scroller = createHorizontalScrollerWithArrows(cardLayout, 430);
    sectionLayout->addWidget(scroller);

    return sectionWidget;
}

void LibraryPageWidget::addPlaceholderMessage(QHBoxLayout* layout, const QString& message)
{
    if (!layout) return;

    clearLayout(layout);

    QLabel* placeholder = new QLabel(message);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    placeholder->setStyleSheet(QString(
                                   "QLabel { color: %1; font-size: 15px; font-style: italic; "
                                   "min-height: 150px; padding: 20px; "
                                   "background-color: transparent; border: none; }"
                                   ).arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    layout->addWidget(placeholder);
}

void LibraryPageWidget::loadLibraryData()
{
    clearLayout(m_watchlistLayout);
    clearLayout(m_favoritesLayout);
    clearLayout(m_historyLayout);
    if(m_customListsLayout) clearLayout(m_customListsLayout);
    m_loadedMediaIds.clear();
    m_statusLabel->hide();

    if (!UserManager::instance()->isAuthenticated()) {
        m_statusLabel->setText("🔐 Please log in to access your personal library");
        m_statusLabel->show();
        updateLibraryStats(0, 0, 0);
        return;
    }

    QJsonObject profile = UserManager::instance()->getProfileData();
    if (profile.isEmpty()) {
        m_statusLabel->setText("⚠️ Unable to load your library. Please try refreshing the page.");
        m_statusLabel->show();
        updateLibraryStats(0, 0, 0);
        return;
    }

    QJsonArray watchlist = profile["watchlist"].toArray();
    if (watchlist.isEmpty()) {
        addPlaceholderMessage(m_watchlistLayout, "Your watchlist is empty. Find movies to add!");
    } else {
        for (const QJsonValue &value : watchlist) {
            fetchMediaDetails(value, "watchlist", m_watchlistLayout);
        }
    }

    QJsonArray favorites = profile["favorites"].toArray();
    if (favorites.isEmpty()) {
        addPlaceholderMessage(m_favoritesLayout, "You haven't added any favorites yet.");
    } else {
        for (const QJsonValue &value : favorites) {
            fetchMediaDetails(value, "favorites", m_favoritesLayout);
        }
    }

    QJsonArray watchedHistory = profile["watchedHistory"].toArray();
    if (watchedHistory.isEmpty()) {
        addPlaceholderMessage(m_historyLayout, "Your watch history is empty.");
    } else {
        for (const QJsonValue &value : watchedHistory) {
            fetchMediaDetails(value, "history", m_historyLayout);
        }
    }

    QJsonObject customLists = profile["customLists"].toObject();
    for (auto it = customLists.constBegin(); it != customLists.constEnd(); ++it) {
        QString listName = it.key();
        QJsonArray movies = it.value().toArray();
        QHBoxLayout* customListLayout = nullptr;
        QWidget* section = createSection(listName, customListLayout, "playlist", true);
        m_customListsLayout->addWidget(section);

        if (movies.isEmpty()) {
            addPlaceholderMessage(customListLayout, QString("This list ('%1') is empty.").arg(listName));
        } else {
            for (const QJsonValue &value : movies) {
                fetchMediaDetails(value, listName, customListLayout);
            }
        }
    }

    updateLibraryStats(watchlist.size(), favorites.size(), watchedHistory.size());
}

void LibraryPageWidget::updateLibraryStats(int watchlistCount, int favoritesCount, int historyCount)
{
    if (m_watchlistCountLabel) m_watchlistCountLabel->setText(QString::number(watchlistCount));
    if (m_favoritesCountLabel) m_favoritesCountLabel->setText(QString::number(favoritesCount));
    if (m_historyCountLabel) m_historyCountLabel->setText(QString::number(historyCount));
}

void LibraryPageWidget::fetchMediaDetails(const QJsonValue &mediaValue, const QString &listType, QHBoxLayout *layout)
{
    int mediaId = mediaValue.toInt();
    if (mediaId == 0 || !layout) return;

    if (m_loadedMediaIds.contains(listType) && m_loadedMediaIds[listType].contains(mediaId)) {
        return;
    }
    m_loadedMediaIds[listType].append(mediaId);

    QString mediaType = "movie";
    QString urlStr = QString("https://api.themoviedb.org/3/%1/%2?api_key=%3").arg(mediaType).arg(mediaId).arg(m_apiKey);

    QNetworkRequest request{QUrl(urlStr)};
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, listType, layout]() {
        processMediaDetailsReply(reply, listType, layout);
    });
}

void LibraryPageWidget::processMediaDetailsReply(QNetworkReply *reply, const QString &listType, QHBoxLayout *layout)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject mediaObj = doc.object();

        if (!mediaObj.isEmpty() && layout) {
            QWidget* cardWidget = createMovieCardWidget(mediaObj, listType);
            layout->addWidget(cardWidget);
        }
    } else {
        qWarning() << "LibraryPage: Failed to fetch media details:" << reply->errorString();
    }
    reply->deleteLater();
}

QWidget* LibraryPageWidget::createMovieCardWidget(const QJsonObject &mediaObj, const QString &listName)
{
    QWidget* container = new QWidget();
    container->setStyleSheet("QWidget { background: transparent; border: none; border-radius: 12px; }");

    QVBoxLayout* vLayout = new QVBoxLayout(container);
    vLayout->setContentsMargins(8, 8, 8, 12);
    vLayout->setSpacing(10);

    MovieCard *card = new MovieCard(mediaObj, "https://image.tmdb.org/t/p/", "w342", container);
    card->setStyleSheet("MovieCard { border-radius: 8px; }");

    connect(card, &MovieCard::clicked, this, [this, mediaObj]() {
        int id = mediaObj["id"].toInt();
        emit movieClicked(id, "movie");
    });

    QPushButton* removeButton = new QPushButton(" Remove");
    removeButton->setIcon(QIcon(":/assets/icons/x"));
    removeButton->setIconSize(QSize(12, 12));
    removeButton->setCursor(Qt::PointingHandCursor);
    removeButton->setStyleSheet(QString(
                                    "QPushButton { color: %1; border: 1px solid rgba(255,255,255,0.1); "
                                    "background: rgba(255,255,255,0.05); font-size: 12px; font-weight: 500; "
                                    "padding: 6px 12px; border-radius: 6px; }"
                                    "QPushButton:hover { color: %2; background: rgba(220,20,60,0.1); "
                                    "border-color: %2; }"
                                    ).arg(AppTheme::TEXT_ON_DARK_SECONDARY, AppTheme::PRIMARY_RED));

    connect(removeButton, &QPushButton::clicked, this, [this, mediaObj, listName](){
        int mediaId = mediaObj["id"].toInt();
        UserManager::instance()->removeMovieFromList(mediaId, listName);
    });

    vLayout->addWidget(card);
    vLayout->addWidget(removeButton, 0, Qt::AlignHCenter);

    return container;
}

void LibraryPageWidget::clearLayout(QLayout *layout)
{
    if (!layout) return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void LibraryPageWidget::onCreateNewList()
{
    bool ok;
    QString listName = QInputDialog::getText(this, tr("Create Custom List"),
                                             tr("Enter a name for your new list:"), QLineEdit::Normal,
                                             "", &ok);
    if (ok && !listName.isEmpty()) {
        UserManager::instance()->createCustomList(listName);
    }
}

QWidget *LibraryPageWidget::createHorizontalScrollerWithArrows(QHBoxLayout *&contentLayout, int minHeight)
{
    QWidget *container = new QWidget();
    container->setMinimumHeight(minHeight);
    container->installEventFilter(this);
    container->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *contentWidget = new QWidget();
    contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 8, 0, 8);
    contentLayout->setSpacing(16);
    contentLayout->setAlignment(Qt::AlignLeft);
    scrollArea->setWidget(contentWidget);

    auto createArrowButton = [&](const QString &iconPath, bool isLeft) {
        QPushButton *btn = new QPushButton(container);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(18, 18));
        btn->setFixedSize(44, 44);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("scrollArrowButton");
        btn->setStyleSheet(QString(
            "QPushButton#scrollArrowButton { "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "stop:0 rgba(255,255,255,0.1), stop:1 rgba(255,255,255,0.05)); "
            "border: 1px solid rgba(255,255,255,0.15); "
            "border-radius: 22px; backdrop-filter: blur(10px); }"
            "QPushButton#scrollArrowButton:hover { "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "stop:0 rgba(220,20,60,0.8), stop:1 rgba(220,20,60,0.6)); "
            "border-color: rgba(220,20,60,0.8); }"
            "QPushButton#scrollArrowButton:pressed { "
            "background: rgba(220,20,60,0.9); }"
            ));
        btn->setVisible(false);
        btn->raise();

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(8);
        shadow->setColor(QColor(0, 0, 0, 60));
        shadow->setOffset(0, 2);
        btn->setGraphicsEffect(shadow);

        return btn;
    };

    QPushButton *leftButton = createArrowButton(":/assets/icons/arrow_left", true);
    QPushButton *rightButton = createArrowButton(":/assets/icons/arrow_right", false);

    mainLayout->addWidget(leftButton, 0, Qt::AlignVCenter);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(rightButton, 0, Qt::AlignVCenter);

    container->setProperty("leftButton", QVariant::fromValue<QObject *>(leftButton));
    container->setProperty("rightButton", QVariant::fromValue<QObject *>(rightButton));
    container->setProperty("scrollArea", QVariant::fromValue<QObject *>(scrollArea));

    auto scroll = [scrollArea](int delta) {
        QPropertyAnimation *anim = new QPropertyAnimation(scrollArea->horizontalScrollBar(), "value");
        anim->setDuration(350);
        anim->setStartValue(scrollArea->horizontalScrollBar()->value());
        anim->setEndValue(scrollArea->horizontalScrollBar()->value() + delta);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };

    connect(leftButton, &QPushButton::clicked, this, [scroll, scrollArea]() {
        scroll(-scrollArea->viewport()->width() * 0.8);
    });
    connect(rightButton, &QPushButton::clicked, this, [scroll, scrollArea]() {
        scroll(scrollArea->viewport()->width() * 0.8);
    });

    auto visibilityUpdater = [container, leftButton, rightButton, scrollArea]() {
        if (!container || !leftButton || !rightButton || !scrollArea) return;
        bool show = container->underMouse();
        leftButton->setVisible(show && scrollArea->horizontalScrollBar()->value() > 0);
        rightButton->setVisible(show && scrollArea->horizontalScrollBar()->value() < scrollArea->horizontalScrollBar()->maximum());
    };

    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, visibilityUpdater);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, visibilityUpdater);
    QTimer::singleShot(0, this, visibilityUpdater);

    return container;
}

bool LibraryPageWidget::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *container = qobject_cast<QWidget *>(watched);
    if (container && container->property("scrollArea").isValid()) {
        QPushButton *leftButton = qobject_cast<QPushButton *>(container->property("leftButton").value<QObject *>());
        QPushButton *rightButton = qobject_cast<QPushButton *>(container->property("rightButton").value<QObject *>());
        QScrollArea *scrollArea = qobject_cast<QScrollArea *>(container->property("scrollArea").value<QObject *>());

        if (leftButton && rightButton && scrollArea) {
            auto updateVisibility = [&]() {
                bool show = container->underMouse();
                leftButton->setVisible(show && scrollArea->horizontalScrollBar()->value() > 0);
                rightButton->setVisible(show && scrollArea->horizontalScrollBar()->value() < scrollArea->horizontalScrollBar()->maximum());
            };

            if (event->type() == QEvent::Enter || event->type() == QEvent::Leave || event->type() == QEvent::MouseMove) {
                updateVisibility();
                return false;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
