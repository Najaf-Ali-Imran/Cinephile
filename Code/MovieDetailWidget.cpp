#include "MovieDetailWidget.h"
#include "MovieCard.h"
#include "Theme.h"
#include "UserManager.h"
#include "clickablelabel.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDebug>
#include <QDesktopServices>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedLayout>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

class AddToListDialog : public QDialog
{
public:
    AddToListDialog(const QStringList& allLists, const QStringList& checkedLists, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Add to My Lists");
        setMinimumSize(450, 350);
        setMaximumSize(450, 500);
        setAttribute(Qt::WA_DeleteOnClose);
        setModal(true);

        setStyleSheet(QString(R"(
            QDialog {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 %1, stop:1 %2);
                border-radius: 16px;
                border: 1px solid %3;
            }
            QLabel {
                color: %4;
                background: transparent;
            }
            QCheckBox {
                color: %4;
                font-size: 14px;
                padding: 8px;
                background: transparent;
                border-radius: 6px;
            }
            QCheckBox:hover {
                background-color: rgba(255, 255, 255, 0.05);
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border-radius: 9px;
                border: 2px solid %5;
                background-color: transparent;
            }
            QCheckBox::indicator:checked {
                background-color: %6;
                border-color: %6;
            }
            QCheckBox::indicator:checked:hover {
                background-color: #F40612;
                border-color: #F40612;
            }
            QPushButton {
                background-color: %7;
                color: %4;
                font-size: 14px;
                font-weight: 500;
                padding: 12px 24px;
                border: none;
                border-radius: 8px;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: %8;
            }
            QPushButton:pressed {
                background-color: %9;
            }
        )").arg(AppTheme::INPUT_BG)
                          .arg(AppTheme::MAIN_WINDOW_BG)
                          .arg(AppTheme::BORDER_COLOR)
                          .arg(AppTheme::TEXT_ON_DARK)
                          .arg(AppTheme::BORDER_COLOR)
                          .arg(AppTheme::PRIMARY_RED)
                          .arg(AppTheme::INPUT_BG_HOVER)
                          .arg(AppTheme::BORDER_COLOR)
                          .arg(AppTheme::INPUT_BG));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(30, 30, 30, 30);
        mainLayout->setSpacing(20);

        QLabel* titleLabel = new QLabel("Select Lists");
        titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: white; margin-bottom: 10px;");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        QLabel* subtitleLabel = new QLabel("Choose which lists to add this movie to:");
        subtitleLabel->setStyleSheet("font-size: 14px; color: #CCCCCC; margin-bottom: 15px;");
        subtitleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(subtitleLabel);

        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

        QWidget *scrollWidget = new QWidget();
        QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
        scrollLayout->setSpacing(8);
        scrollLayout->setContentsMargins(5, 5, 5, 5);

        for (const QString &listName : allLists) {
            QCheckBox *checkbox = new QCheckBox(listName);
            checkbox->setChecked(checkedLists.contains(listName));
            m_checkboxes[listName] = checkbox;
            scrollLayout->addWidget(checkbox);
        }

        scrollArea->setWidget(scrollWidget);
        scrollArea->setMaximumHeight(200);
        mainLayout->addWidget(scrollArea);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(15);

        QPushButton *cancelBtn = new QPushButton("Cancel");
        QPushButton *saveBtn = new QPushButton("Save Changes");

        saveBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #F40612;
            }
        )").arg(AppTheme::PRIMARY_RED));

        buttonLayout->addStretch();
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(saveBtn);

        mainLayout->addLayout(buttonLayout);

        connect(cancelBtn, &QPushButton::clicked, this, &AddToListDialog::reject);
        connect(saveBtn, &QPushButton::clicked, this, &AddToListDialog::accept);
    }

    QStringList getSelectedLists() const
    {
        QStringList selected;
        for (auto it = m_checkboxes.constBegin(); it != m_checkboxes.constEnd(); ++it) {
            if (it.value()->isChecked()) {
                selected << it.key();
            }
        }
        return selected;
    }

private:
    QMap<QString, QCheckBox*> m_checkboxes;
};

MovieDetailWidget::MovieDetailWidget(const QString &apiKey, QWidget *parent)
    : QWidget(parent)
    , m_pageLayout(new QStackedLayout(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentItemId(-1)
    , m_apiKey(apiKey)
    , m_addToListButton(nullptr)
    , m_profileSize("w185")
{
    setupUI();
    connect(UserManager::instance(), &UserManager::profileDataChanged, this, &MovieDetailWidget::updateAddToListButtonState);
}

MovieDetailWidget::~MovieDetailWidget() {}

void MovieDetailWidget::setupUI()
{
    this->setStyleSheet(QString("background-color: %1;").arg(AppTheme::MAIN_WINDOW_BG));
    m_pageLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *loadingWidget = new QWidget();
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingWidget);
    QLabel *loadingLabel = new QLabel("Loading details...");
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet(QString("color: %1; font-size: 20px;").arg(AppTheme::TEXT_ON_DARK));
    loadingLayout->addWidget(loadingLabel);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background-color: transparent;");
    m_scrollArea->setWidget(m_contentWidget);

    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_pageLayout->addWidget(loadingWidget);
    m_pageLayout->addWidget(m_scrollArea);
    m_pageLayout->setCurrentWidget(loadingWidget);
}

void MovieDetailWidget::clearUI()
{
    m_contentWidget->setVisible(false);
    m_addToListButton = nullptr;

    QLayoutItem *item;
    while ((item = m_mainLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void MovieDetailWidget::loadDetails(int itemId, const QString &itemType)
{
    if (itemId == m_currentItemId && m_pageLayout->currentWidget() == m_scrollArea) {
        return;
    }

    m_pageLayout->setCurrentWidget(m_pageLayout->widget(0));
    m_currentItemId = itemId;
    m_currentItemType = itemType;
    if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
        m_scrollArea->verticalScrollBar()->setValue(0);
    }

    QTimer::singleShot(0, this, [this]() {
        clearUI();
        fetchDetails(m_currentItemId, m_currentItemType);
    });
}

void MovieDetailWidget::fetchDetails(int itemId, const QString &itemType)
{
    QUrl url(QString("https://api.themoviedb.org/3/%1/%2").arg(itemType).arg(itemId));
    QUrlQuery query;
    query.addQueryItem("api_key", m_apiKey);
    query.addQueryItem("append_to_response", "credits,videos,recommendations");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            populateUI(doc.object());
        } else {
            qWarning() << "Failed to fetch details:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void MovieDetailWidget::populateUI(const QJsonObject &data)
{
    QWidget *heroSectionContainer = new QWidget();
    heroSectionContainer->setStyleSheet("background-color: transparent;");
    heroSectionContainer->setMinimumHeight(550);

    QStackedLayout *heroStack = new QStackedLayout(heroSectionContainer);
    heroStack->setContentsMargins(0, 0, 0, 0);
    heroStack->setStackingMode(QStackedLayout::StackAll);

    QLabel *blurredBackdropLabel = new QLabel();
    blurredBackdropLabel->setScaledContents(true);
    blurredBackdropLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    blurredBackdropLabel->setStyleSheet("background-color: #111;");
    auto *blurEffect = new QGraphicsBlurEffect();
    blurEffect->setBlurRadius(25);
    blurredBackdropLabel->setGraphicsEffect(blurEffect);

    QLabel *sharpBackdropLabel = new QLabel();
    sharpBackdropLabel->setScaledContents(true);
    sharpBackdropLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sharpBackdropLabel->setStyleSheet("background-color: #1a1a1a;");

    auto *fadeEffect = new QGraphicsOpacityEffect();
    fadeEffect->setOpacity(0.0);
    sharpBackdropLabel->setGraphicsEffect(fadeEffect);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(fadeEffect, "opacity");
    fadeIn->setDuration(1000);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
    fadeIn->setParent(this);

    QLabel *gradientOverlay = new QLabel();
    gradientOverlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gradientOverlay->setStyleSheet(QLatin1String(R"(
        background: qlineargradient(
            x1: 0, y1: 0, x2: 0, y2: 1,
            stop: 0.0 rgba(0,0,0,0.7),
            stop: 0.3 rgba(0,0,0,0.5),
            stop: 0.7 rgba(17,17,17,0.8),
            stop: 1.0 %1)
    )").arg(AppTheme::MAIN_WINDOW_BG));

    QWidget *heroContentWidget = new QWidget();
    heroContentWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *heroLayout = new QHBoxLayout(heroContentWidget);
    heroLayout->setContentsMargins(60, 80, 60, 40);
    heroLayout->setSpacing(40);

    heroStack->addWidget(blurredBackdropLabel);
    heroStack->addWidget(sharpBackdropLabel);
    heroStack->addWidget(gradientOverlay);
    heroStack->addWidget(heroContentWidget);
    heroContentWidget->raise();

    m_mainLayout->addWidget(heroSectionContainer);

    QString backdropPath = data["backdrop_path"].toString();
    if (!backdropPath.isEmpty()) {
        QUrl url(m_imageBaseUrl + m_backdropSize + backdropPath);
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                if (pixmap.loadFromData(reply->readAll())) {
                    QPixmap scaled = pixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    blurredBackdropLabel->setPixmap(scaled);
                    sharpBackdropLabel->setPixmap(scaled);
                    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
            reply->deleteLater();
        });
    }

    QLabel *poster = new QLabel();
    poster->setFixedSize(250, 375);
    poster->setScaledContents(true);
    poster->setStyleSheet("border-radius: 12px; background-color: #333; border: none;");

    auto *posterShadow = new QGraphicsDropShadowEffect();
    posterShadow->setBlurRadius(20);
    posterShadow->setColor(QColor(0, 0, 0, 120));
    posterShadow->setOffset(0, 8);
    poster->setGraphicsEffect(posterShadow);

    QString posterPath = data["poster_path"].toString();
    if (!posterPath.isEmpty()) {
        QUrl url(m_imageBaseUrl + m_posterSize + posterPath);
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                poster->setPixmap(pixmap.scaled(poster->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            reply->deleteLater();
        });
    }

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(15);
    infoLayout->addStretch();

    QLabel *title = new QLabel(data.contains("title") ? data["title"].toString() : data["name"].toString());
    title->setStyleSheet("color: white; font-size: 36px; font-weight: bold; text-shadow: 2px 2px 4px rgba(0,0,0,0.8);");
    title->setWordWrap(true);

    QLabel *tagline = new QLabel(data["tagline"].toString());
    tagline->setStyleSheet("color: #E0E0E0; font-style: italic; font-size: 18px; text-shadow: 1px 1px 2px rgba(0,0,0,0.8);");
    tagline->setWordWrap(true);

    QString ratingStr = data.contains("vote_average") ? QString::number(data["vote_average"].toDouble(), 'f', 1) : "N/A";
    QString runtimeStr;
    if (data.contains("runtime")) runtimeStr = QString::number(data["runtime"].toInt()) + " min";
    else if (data.contains("episode_run_time")) {
        QJsonArray arr = data["episode_run_time"].toArray();
        if (!arr.isEmpty()) runtimeStr = QString::number(arr.at(0).toInt()) + " min";
    }
    QLabel *meta = new QLabel("⭐ " + ratingStr + "    ⏱️ " + runtimeStr);
    meta->setStyleSheet("color: white; font-size: 16px; font-weight: 500; text-shadow: 1px 1px 2px rgba(0,0,0,0.8);");

    QLabel *overview = new QLabel(data["overview"].toString());
    overview->setStyleSheet("color: #F0F0F0; font-size: 16px; line-height: 1.5; text-shadow: 1px 1px 2px rgba(0,0,0,0.8);");
    overview->setWordWrap(true);
    overview->setMaximumWidth(600);

    // === Buttons ===
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(20);

    QPushButton *watchNow = new QPushButton("▶ Watch Now");
    m_addToListButton = new QPushButton("+ Add to List");

    for (QPushButton *btn : {watchNow, m_addToListButton}) {
        btn->setMinimumHeight(45);
        btn->setCursor(Qt::PointingHandCursor);
    }

    watchNow->setStyleSheet(
        "QPushButton { background-color: #E50914; color: white; border: none; "
        "border-radius: 8px; padding: 0 30px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #F40612; }");

    m_addToListButton->setStyleSheet(
        "QPushButton { background-color: #2b2b2b; color: white; border: 1px solid gray; "
        "border-radius: 8px; padding: 0 30px; font-size: 16px; font-weight: 500; }"
        "QPushButton:hover { background-color: #3a3a3a; border-color: white; }");

    connect(watchNow, &QPushButton::clicked, this, [this, data]() {
        int tmdbId = data["id"].toInt();
        if (tmdbId <= 0) {
            return;
        }

        if (UserManager::instance()->isAuthenticated()) {
            qDebug() << "User is authenticated. Adding movie" << tmdbId << "to Watch History.";
            UserManager::instance()->addMovieToList(tmdbId, "watchedHistory");
        } else {
            qDebug() << "User not authenticated. Skipping adding to Watch History.";
        }

        // Open the stream URL
        QUrl url(QString("https://vidlink.pro/movie/%1").arg(tmdbId));
        QUrlQuery query;
        query.addQueryItem("primaryColor", "000000");
        query.addQueryItem("secondaryColor", "EEEBDd");
        query.addQueryItem("iconColor", "CE1212");
        query.addQueryItem("autoplay", "true");
        url.setQuery(query);
        QDesktopServices::openUrl(url);
    });

    connect(m_addToListButton, &QPushButton::clicked, this, &MovieDetailWidget::onAddToListClicked);
    updateAddToListButtonState();

    buttonsLayout->addWidget(watchNow);
    buttonsLayout->addWidget(m_addToListButton);
    buttonsLayout->addStretch();

    infoLayout->addWidget(title);
    if (!tagline->text().isEmpty()) {
        infoLayout->addWidget(tagline);
    }
    infoLayout->addWidget(meta);
    infoLayout->addSpacing(20);
    infoLayout->addWidget(overview);
    infoLayout->addSpacing(25);
    infoLayout->addLayout(buttonsLayout);
    infoLayout->addStretch();

    // === Assemble Final Hero Layout ===
    heroLayout->addWidget(poster, 0, Qt::AlignTop);
    heroLayout->addLayout(infoLayout);

    auto addSection = [&](const QString &title) {
        QWidget *sectionHeader = new QWidget();
        sectionHeader->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + "; padding: 20px 0;");
        QVBoxLayout *headerLayout = new QVBoxLayout(sectionHeader);
        headerLayout->setContentsMargins(60, 20, 60, 10);

        QLabel *label = new QLabel(title);
        label->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
        headerLayout->addWidget(label);

        m_mainLayout->addWidget(sectionHeader);
    };

    if (data.contains("credits") && data["credits"].toObject().contains("cast")) {
        addSection("Cast");
        QHBoxLayout *castLayout = new QHBoxLayout();
        m_castLayout = castLayout;
        QWidget *scroller = createHorizontalScrollerWithArrows(castLayout, 300);
        scroller->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
        m_mainLayout->addWidget(scroller);
        populateCast(data["credits"].toObject()["cast"].toArray());
    }

    if (data.contains("videos") && data["videos"].toObject().contains("results")) {
        addSection("Trailers & Videos");
        QHBoxLayout *videosLayout = new QHBoxLayout();
        m_videosLayout = videosLayout;
        QWidget *scroller = createHorizontalScrollerWithArrows(videosLayout, 240);
        scroller->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
        m_mainLayout->addWidget(scroller);
        populateVideos(data["videos"].toObject()["results"].toArray());
    }

    if (data.contains("recommendations") && data["recommendations"].toObject().contains("results")) {
        addSection("Recommendations");
        QHBoxLayout *recsLayout = new QHBoxLayout();
        m_recommendationsLayout = recsLayout;
        QWidget *scroller = createHorizontalScrollerWithArrows(recsLayout, 300);
        scroller->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
        m_mainLayout->addWidget(scroller);
        populateRecommendations(data["recommendations"].toObject()["results"].toArray());
    }

    QWidget *spacer = new QWidget();
    spacer->setMinimumHeight(50);
    spacer->setStyleSheet("background-color: " + AppTheme::MAIN_WINDOW_BG + ";");
    m_mainLayout->addWidget(spacer);

    m_pageLayout->setCurrentWidget(m_scrollArea);
    m_contentWidget->setVisible(true);
}

void MovieDetailWidget::onAddToListClicked()
{
    if (!UserManager::instance()->isAuthenticated() || m_currentItemId == -1) {
        return;
    }

    QStringList allLists = UserManager::instance()->getAllListNames();
    QStringList checkedLists = UserManager::instance()->getListsForMovie(m_currentItemId);

    // AFTER (creates on heap)
    AddToListDialog *dialog = new AddToListDialog(allLists, checkedLists, this);
    if (dialog->exec() == QDialog::Accepted) {
        QStringList finalList = dialog->getSelectedLists();
        UserManager::instance()->updateMovieInLists(m_currentItemId, finalList);
    }
}

void MovieDetailWidget::updateAddToListButtonState()
{
    if (!m_addToListButton || !UserManager::instance()->isAuthenticated() || m_currentItemId == -1) {
        if(m_addToListButton) m_addToListButton->setText("+ Add to List");
        return;
    }

    QStringList lists = UserManager::instance()->getListsForMovie(m_currentItemId);
    if (lists.isEmpty()) {
        m_addToListButton->setText("+ Add to List");
        m_addToListButton->setIcon(QIcon());
    } else {
        m_addToListButton->setText("✓ In My Lists");
        m_addToListButton->setIcon(QIcon(":/assets/icons/check.svg"));
    }
}

void MovieDetailWidget::populateCast(const QJsonArray &castArray)
{
    for (int i = 0; i < qMin(castArray.size(), 15); ++i) {
        QJsonObject actor = castArray[i].toObject();
        if (actor["profile_path"].isNull())
            continue;

        QWidget *card = new QWidget();
        card->setStyleSheet("background-color: transparent;");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(8);

        QLabel *photo = new QLabel();
        photo->setFixedSize(150, 225);
        photo->setScaledContents(true);
        photo->setStyleSheet("background-color: #333; border-radius: 8px;");

        QUrl photoUrl(m_imageBaseUrl + m_profileSize + actor["profile_path"].toString());
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(photoUrl));
        connect(reply, &QNetworkReply::finished, this, [reply, photo]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                photo->setPixmap(pixmap);
            }
            reply->deleteLater();
        });

        QLabel *name = new QLabel(actor["name"].toString());
        name->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;").arg(AppTheme::TEXT_ON_DARK));
        name->setWordWrap(true);
        name->setAlignment(Qt::AlignCenter);

        QLabel *character = new QLabel(actor["character"].toString());
        character->setStyleSheet(QString("color: %1; font-size: 12px;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
        character->setWordWrap(true);
        character->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(photo);
        cardLayout->addWidget(name);
        cardLayout->addWidget(character);
        cardLayout->addStretch();

        m_castLayout->addWidget(card);
    }
    if (m_castLayout) m_castLayout->addStretch();
}

void MovieDetailWidget::populateVideos(const QJsonArray &videosArray)
{
    for (const auto &videoVal : videosArray) {
        QJsonObject video = videoVal.toObject();
        if (video["site"].toString().toLower() != "youtube")
            continue;

        ClickableLabel *card = new ClickableLabel();
        card->setFixedSize(280, 200);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet("background-color: transparent;");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(8);
        QStackedLayout *imageStack = new QStackedLayout();

        QLabel *thumbnail = new QLabel();
        thumbnail->setScaledContents(true);
        thumbnail->setStyleSheet("background-color: #000; border-radius: 8px;");

        QUrl thumbnailUrl(QString("https://img.youtube.com/vi/%1/hqdefault.jpg").arg(video["key"].toString()));
        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(thumbnailUrl));
        connect(reply, &QNetworkReply::finished, this, [reply, thumbnail]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                thumbnail->setPixmap(pixmap);
            }
            reply->deleteLater();
        });

        QLabel *playIcon = new QLabel("▶");
        playIcon->setAlignment(Qt::AlignCenter);
        playIcon->setStyleSheet("color: white; font-size: 40px; background-color: rgba(0,0,0,0.3); border-radius: 8px;");

        imageStack->addWidget(thumbnail);
        imageStack->addWidget(playIcon);

        QLabel *title = new QLabel(video["name"].toString());
        title->setWordWrap(true);
        title->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 500;").arg(AppTheme::TEXT_ON_DARK));
        title->setAlignment(Qt::AlignCenter);

        cardLayout->addLayout(imageStack);
        cardLayout->addWidget(title);

        connect(card, &ClickableLabel::clicked, this, [key = video["key"].toString()]() {
            QDesktopServices::openUrl(QUrl(QString("https://www.youtube.com/watch?v=%1").arg(key)));
        });

        m_videosLayout->addWidget(card);
    }
    if (m_videosLayout) m_videosLayout->addStretch();
}

void MovieDetailWidget::populateRecommendations(const QJsonArray &recsArray)
{
    for (const auto &recVal : recsArray) {
        QJsonObject rec = recVal.toObject();
        if (rec["poster_path"].isNull())
            continue;

        MovieCard *card = new MovieCard(rec, m_imageBaseUrl, "w400", this);
        card->setMinimumHeight(240);
        connect(card, &MovieCard::clicked, this, [this, rec]() {
            loadDetails(rec["id"].toInt(), rec.contains("title") ? "movie" : "tv");
        });
        m_recommendationsLayout->addWidget(card);
    }
    if (m_recommendationsLayout) m_recommendationsLayout->addStretch();
}


QWidget *MovieDetailWidget::createHorizontalScrollerWithArrows(QHBoxLayout *contentLayout,
                                                               int minHeight)
{
    QWidget *container = new QWidget();
    container->setMinimumHeight(minHeight);
    container->installEventFilter(this);

    container->setContentsMargins(40, 0, 40, 0);

    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent; border: none;");

    QWidget *contentWidget = new QWidget();
    contentWidget->setLayout(contentLayout);
    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);

    mainLayout->addWidget(scrollArea);

    auto createArrowButton = [&](const QString &iconPath) {
        QPushButton *btn = new QPushButton(container);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(20, 20));
        btn->setFixedSize(45, 45);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("scrollArrowButton");
        btn->setStyleSheet(
            "QPushButton#scrollArrowButton { background-color: rgba(20, 20, 20, 0.7); "
            "border-radius: 22px; border: none; }"
            "QPushButton#scrollArrowButton:hover { background-color: rgba(0, 0, 0, 0.9); }");
        btn->setVisible(false);
        btn->raise();
        return btn;
    };

    QPushButton *leftButton = createArrowButton(":/assets/icons/arrow_left.svg");
    QPushButton *rightButton = createArrowButton(":/assets/icons/arrow_right.svg");

    container->setProperty("leftButton", QVariant::fromValue<QObject *>(leftButton));
    container->setProperty("rightButton", QVariant::fromValue<QObject *>(rightButton));
    container->setProperty("scrollArea", QVariant::fromValue<QObject *>(scrollArea));

    connect(leftButton, &QPushButton::clicked, this, [scrollArea]() {
        QPropertyAnimation *anim = new QPropertyAnimation(scrollArea->horizontalScrollBar(), "value");
        anim->setDuration(400);
        anim->setStartValue(scrollArea->horizontalScrollBar()->value());
        anim->setEndValue(scrollArea->horizontalScrollBar()->value() - scrollArea->viewport()->width() * 0.9);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });
    connect(rightButton, &QPushButton::clicked, this, [scrollArea]() {
        QPropertyAnimation *anim = new QPropertyAnimation(scrollArea->horizontalScrollBar(), "value");
        anim->setDuration(400);
        anim->setStartValue(scrollArea->horizontalScrollBar()->value());
        anim->setEndValue(scrollArea->horizontalScrollBar()->value() + scrollArea->viewport()->width() * 0.9);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });

    auto visibilityUpdater = [container, leftButton, rightButton, scrollArea]() {
        bool show = container->underMouse();
        leftButton->setVisible(show && scrollArea->horizontalScrollBar()->value() > 0);
        rightButton->setVisible(show && scrollArea->horizontalScrollBar()->value() < scrollArea->horizontalScrollBar()->maximum());
    };

    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, visibilityUpdater);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, visibilityUpdater);
    QTimer::singleShot(0, this, visibilityUpdater);

    return container;
}

bool MovieDetailWidget::eventFilter(QObject *watched, QEvent *event)
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

            if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
                leftButton->move(0, (container->height() - leftButton->height()) / 2);
                rightButton->move(container->width() - rightButton->width(), (container->height() - rightButton->height()) / 2);
            }

            if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
                updateVisibility();
                return false;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
