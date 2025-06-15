#include "ProfilePageWidget.h"
#include "FirestoreService.h"
#include "LoginWidget.h"
#include "Theme.h"
#include "UserManager.h"
#include "clickablelabel.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QVBoxLayout>

ProfilePageWidget::ProfilePageWidget(LoginWidget *authService,
                                     FirestoreService *firestoreService,
                                     QWidget *parent)
    : QWidget(parent)
    , m_authService(authService)
    , m_firestoreService(firestoreService)
{
    setObjectName("profilePageWidget");
    setupUi();

    connect(UserManager::instance(),
            &UserManager::displayNameChanged,
            this,
            &ProfilePageWidget::updateUIFromUserManager);
    connect(UserManager::instance(),
            &UserManager::emailChanged,
            this,
            &ProfilePageWidget::updateUIFromUserManager);
    connect(UserManager::instance(),
            &UserManager::profilePictureChanged,
            this,
            &ProfilePageWidget::updateUIFromUserManager);

    if (m_authService) {
        connect(m_authService,
                &LoginWidget::profileUpdateSuccess,
                this,
                &ProfilePageWidget::handleProfileAuthUpdateSuccess);
        connect(m_authService,
                &LoginWidget::profileUpdateFailed,
                this,
                &ProfilePageWidget::handleProfileAuthUpdateFailed);
    } else {
        qWarning() << "LoginWidget is null";
    }

    if (m_firestoreService) {
        connect(m_firestoreService,
                &FirestoreService::userDocumentUpdated,
                this,
                &ProfilePageWidget::handleFirestoreUpdateSuccess);
    } else {
        qWarning() << "FirestoreService is null";
    }
}

ProfilePageWidget::~ProfilePageWidget() { }

QString ProfilePageWidget::getProfilePicStoragePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataPath.isEmpty()) {
        dataPath = QDir::homePath() + "/.CinephileAppData";
    }
    QDir dir(dataPath);
    if (!dir.exists("profile_pictures")) {
        dir.mkpath("profile_pictures");
    }
    return dataPath + "/profile_pictures";
}

QString ProfilePageWidget::formatDisplayNameForStorage(const QString &name)
{
    QString formattedName = name.trimmed();
    return formattedName;
}

QWidget *ProfilePageWidget::createSectionHeader(const QString &title)
{
    QLabel *header = new QLabel(title, m_contentWidget);
    header->setObjectName("profileSectionGroupHeader");
    header->setStyleSheet(QString("QLabel#profileSectionGroupHeader { color: %1; font-size: 20px; "
                                  "font-weight: bold; margin-top: 15px; margin-bottom: 10px; }")
                              .arg(AppTheme::TEXT_ON_DARK));
    return header;
}

QFrame *ProfilePageWidget::createDivider()
{
    QFrame *divider = new QFrame(m_contentWidget);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    divider->setObjectName("profileDivider");
    divider->setStyleSheet(
        QString("QFrame#profileDivider { background-color: %1; border: none; min-height: 1px; "
                "max-height: 1px; margin-top:10px; margin-bottom:10px;}")
            .arg(AppTheme::SKELETON_BG));
    return divider;
}

QWidget *ProfilePageWidget::createEditableField(const QString &labelText,
                                                QLineEdit **fieldLineEdit,
                                                QPushButton **button,
                                                const QString &buttonText,
                                                const QString &placeholder)
{
    QWidget *container = new QWidget(m_contentWidget);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QLabel *fieldLabel = new QLabel(labelText, container);
    fieldLabel->setObjectName("profileFieldLabel");
    fieldLabel->setStyleSheet(QString("QLabel#profileFieldLabel { color: %1; font-size: 14px; }")
                                  .arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    *fieldLineEdit = new QLineEdit(container);
    (*fieldLineEdit)->setObjectName("profileLineEdit");
    (*fieldLineEdit)->setPlaceholderText(placeholder.isEmpty() ? labelText : placeholder);
    (*fieldLineEdit)
        ->setStyleSheet(QString("QLineEdit#profileLineEdit {"
                                "   background-color: %1;"
                                "   color: %2;"
                                "   border: 1px solid %3;"
                                "   border-radius: 6px;"
                                "   padding: 8px 10px;"
                                "   font-size: 15px;"
                                "}"
                                "QLineEdit#profileLineEdit:focus {"
                                "   border-color: %4;"
                                "}")
                            .arg(AppTheme::INPUT_BG,
                                 AppTheme::TEXT_ON_DARK,
                                 AppTheme::SKELETON_BG,
                                 AppTheme::PRIMARY_RED));

    *button = new QPushButton(buttonText, container);
    (*button)->setObjectName("profileSaveButton");
    (*button)->setMinimumHeight(38);
    (*button)->setCursor(Qt::PointingHandCursor);
    (*button)->setStyleSheet(QString("QPushButton#profileSaveButton {"
                                     "   background-color: %1;"
                                     "   color: %2;"
                                     "   border: none;"
                                     "   border-radius: 6px;"
                                     "   padding: 8px 15px;"
                                     "   font-size: 14px;"
                                     "   font-weight: 500;"
                                     "   min-width: 100px;"
                                     "}"
                                     "QPushButton#profileSaveButton:hover {"
                                     "   background-color: %3;"
                                     "}"
                                     "QPushButton#profileSaveButton:disabled {"
                                     "   background-color: %4;"
                                     "   color: %5;"
                                     "}")
                                 .arg(AppTheme::PRIMARY_RED,
                                      AppTheme::BUTTON_WHITE_BG,
                                      AppTheme::BUTTON_HOVER_LIGHT_BG,
                                      AppTheme::SKELETON_BG,
                                      AppTheme::TEXT_DISABLED));

    QHBoxLayout *fieldAndButtonLayout = new QHBoxLayout();
    fieldAndButtonLayout->addWidget(*fieldLineEdit, 1);
    fieldAndButtonLayout->addWidget(*button);

    layout->addWidget(fieldLabel);
    layout->addLayout(fieldAndButtonLayout);

    return container;
}

void ProfilePageWidget::setupUi()
{
    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("profileScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        QString("QScrollArea#profileScrollArea { background-color: %1; border: none; }")
            .arg(AppTheme::MAIN_WINDOW_BG));

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName("profileContentWidget");
    m_contentWidget->setStyleSheet(
        QString("QWidget#profileContentWidget { background-color: transparent; }"));

    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(50, 40, 50, 40);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setAlignment(Qt::AlignTop);

    QWidget *profileHeaderWidget = new QWidget(m_contentWidget);
    QHBoxLayout *profileHeaderLayout = new QHBoxLayout(profileHeaderWidget);
    profileHeaderLayout->setContentsMargins(0, 0, 0, 0);
    profileHeaderLayout->setSpacing(25);

    m_profilePictureLabel = new ClickableLabel(m_contentWidget);
    m_profilePictureLabel->setObjectName("profilePicture");
    m_profilePictureLabel->setFixedSize(120, 120);
    m_profilePictureLabel->setAlignment(Qt::AlignCenter);
    m_profilePictureLabel->setToolTip("Click to change profile picture");
    m_profilePictureLabel->setCursor(Qt::PointingHandCursor);
    m_profilePictureLabel->setStyleSheet(QString("ClickableLabel#profilePicture {"
                                                 "   background-color: %1;"
                                                 "   border: 2px dashed %2;"
                                                 "   border-radius: 60px;"
                                                 "   color: %3;"
                                                 "   font-size: 14px;"
                                                 "}"
                                                 "ClickableLabel#profilePicture:hover {"
                                                 "   border-color: %4;"
                                                 "}")
                                             .arg(AppTheme::INPUT_BG,
                                                  AppTheme::TEXT_ON_DARK_SECONDARY,
                                                  AppTheme::TEXT_ON_DARK_SECONDARY,
                                                  AppTheme::PRIMARY_RED));
    m_profilePictureLabel->setText("No Picture");
    connect(m_profilePictureLabel,
            &ClickableLabel::clicked,
            this,
            &ProfilePageWidget::onProfilePictureClicked);

    QWidget *userInfoWidget = new QWidget(profileHeaderWidget);
    QVBoxLayout *userInfoLayout = new QVBoxLayout(userInfoWidget);
    userInfoLayout->setContentsMargins(0, 0, 0, 0);
    userInfoLayout->setSpacing(5);
    userInfoLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_displayNameHeaderLabel = new QLabel("User Name", userInfoWidget);
    m_displayNameHeaderLabel->setObjectName("profileDisplayNameHeader");
    m_displayNameHeaderLabel->setStyleSheet(
        QString(
            "QLabel#profileDisplayNameHeader { color: %1; font-size: 26px; font-weight: bold; }")
            .arg(AppTheme::TEXT_ON_DARK));
    m_displayNameHeaderLabel->setWordWrap(true);

    m_emailHeaderLabel = new QLabel("user@example.com", userInfoWidget);
    m_emailHeaderLabel->setObjectName("profileEmailHeader");
    m_emailHeaderLabel->setStyleSheet(
        QString("QLabel#profileEmailHeader { color: %1; font-size: 16px; }")
            .arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_emailHeaderLabel->setWordWrap(true);

    userInfoLayout->addWidget(m_displayNameHeaderLabel);
    userInfoLayout->addWidget(m_emailHeaderLabel);
    userInfoLayout->addStretch();

    profileHeaderLayout->addWidget(m_profilePictureLabel);
    profileHeaderLayout->addWidget(userInfoWidget, 1);

    m_mainLayout->addWidget(profileHeaderWidget);
    m_mainLayout->addWidget(createDivider());

    m_mainLayout->addWidget(createSectionHeader("Edit Profile"));
    QWidget *editDisplayNameWidget = createEditableField("Display Name",
                                                         &m_editDisplayNameLineEdit,
                                                         &m_saveDisplayNameButton,
                                                         "Save Name");
    m_mainLayout->addWidget(editDisplayNameWidget);
    connect(m_saveDisplayNameButton,
            &QPushButton::clicked,
            this,
            &ProfilePageWidget::onSaveDisplayNameClicked);

    m_mainLayout->addWidget(createDivider());

    m_mainLayout->addWidget(createSectionHeader("Account Settings"));

    QWidget *changeEmailWidget = new QWidget(m_contentWidget);
    QVBoxLayout *changeEmailLayout = new QVBoxLayout(changeEmailWidget);
    changeEmailLayout->setContentsMargins(0, 0, 0, 0);
    changeEmailLayout->setSpacing(5);
    QLabel *emailLabel = new QLabel("Change Email Address", changeEmailWidget);
    emailLabel->setObjectName("profileFieldLabel");
    emailLabel->setStyleSheet(QString("QLabel#profileFieldLabel { color: %1; font-size: 14px; }")
                                  .arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_editEmailLineEdit = new QLineEdit(changeEmailWidget);
    m_editEmailLineEdit->setObjectName("profileLineEdit");
    m_editEmailLineEdit->setPlaceholderText("New Email Address");

    m_saveEmailButton = new QPushButton("Request Email Change", changeEmailWidget);
    m_saveEmailButton->setObjectName("profileSaveButton");
    m_editEmailLineEdit->setStyleSheet(m_editDisplayNameLineEdit->styleSheet());
    m_saveEmailButton->setStyleSheet(m_saveDisplayNameButton->styleSheet());
    m_saveEmailButton->setMinimumHeight(38);
    m_saveEmailButton->setCursor(Qt::PointingHandCursor);

    changeEmailLayout->addWidget(emailLabel);
    changeEmailLayout->addWidget(m_editEmailLineEdit);

    changeEmailLayout->addWidget(m_saveEmailButton, 0, Qt::AlignLeft);
    m_mainLayout->addWidget(changeEmailWidget);
    connect(m_saveEmailButton, &QPushButton::clicked, this, &ProfilePageWidget::onSaveEmailClicked);

    m_mainLayout->addSpacing(15);

    QWidget *changePasswordWidget = new QWidget(m_contentWidget);
    QVBoxLayout *changePasswordLayout = new QVBoxLayout(changePasswordWidget);
    changePasswordLayout->setContentsMargins(0, 0, 0, 0);
    changePasswordLayout->setSpacing(5);
    QLabel *passwordLabel = new QLabel("Change Password", changePasswordWidget);
    passwordLabel->setObjectName("profileFieldLabel");
    passwordLabel->setStyleSheet(QString("QLabel#profileFieldLabel { color: %1; font-size: 14px; }")
                                     .arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_currentPasswordLineEdit = new QLineEdit(
        changePasswordWidget);
    m_currentPasswordLineEdit->setObjectName("profileLineEdit");
    m_currentPasswordLineEdit->setPlaceholderText("Current Password (for your reference)");
    m_currentPasswordLineEdit->setEchoMode(QLineEdit::Password);

    m_newPasswordLineEdit = new QLineEdit(changePasswordWidget);
    m_newPasswordLineEdit->setObjectName("profileLineEdit");
    m_newPasswordLineEdit->setPlaceholderText("New Password (min. 6 characters)");
    m_newPasswordLineEdit->setEchoMode(QLineEdit::Password);

    m_confirmNewPasswordLineEdit = new QLineEdit(changePasswordWidget);
    m_confirmNewPasswordLineEdit->setObjectName("profileLineEdit");
    m_confirmNewPasswordLineEdit->setPlaceholderText("Confirm New Password");
    m_confirmNewPasswordLineEdit->setEchoMode(QLineEdit::Password);

    m_changePasswordButton = new QPushButton("Change Password", changePasswordWidget);
    m_changePasswordButton->setObjectName("profileSaveButton");

    m_currentPasswordLineEdit->setStyleSheet(m_editDisplayNameLineEdit->styleSheet());

    m_newPasswordLineEdit->setStyleSheet(m_editDisplayNameLineEdit->styleSheet());

    m_confirmNewPasswordLineEdit->setStyleSheet(m_editDisplayNameLineEdit->styleSheet());

    m_changePasswordButton->setStyleSheet(m_saveDisplayNameButton->styleSheet());
    m_changePasswordButton->setMinimumHeight(38);
    m_changePasswordButton->setCursor(Qt::PointingHandCursor);

    changePasswordLayout->addWidget(passwordLabel);
    changePasswordLayout->addWidget(m_currentPasswordLineEdit);
    changePasswordLayout->addWidget(m_newPasswordLineEdit);
    changePasswordLayout->addWidget(m_confirmNewPasswordLineEdit);
    changePasswordLayout->addWidget(m_changePasswordButton, 0, Qt::AlignLeft);

    m_mainLayout->addWidget(changePasswordWidget);
    connect(m_changePasswordButton,
            &QPushButton::clicked,
            this,
            &ProfilePageWidget::onChangePasswordClicked);

    m_mainLayout->addWidget(createDivider());

    m_statusMessageLabel = new QLabel(m_contentWidget);
    m_statusMessageLabel->setObjectName("profileStatusMessage");
    m_statusMessageLabel->setAlignment(Qt::AlignCenter);
    m_statusMessageLabel->setWordWrap(true);
    m_statusMessageLabel->setVisible(false);
    m_mainLayout->addWidget(m_statusMessageLabel);

    m_mainLayout->addStretch(1);
    m_mainLayout->addWidget(createSectionHeader("Actions"));
    m_logoutButton = new QPushButton("Logout", m_contentWidget);
    m_logoutButton->setObjectName("profileLogoutButton");
    m_logoutButton->setMinimumHeight(42);
    m_logoutButton->setCursor(Qt::PointingHandCursor);
    m_logoutButton->setStyleSheet(
        QString("QPushButton#profileLogoutButton {"
                "   background-color: %1;"
                "   color: %2;"
                "   border: none;"
                "   border-radius: 6px;"
                "   padding: 10px 20px;"
                "   font-size: 15px;"
                "   font-weight: 500;"
                "}"
                "QPushButton#profileLogoutButton:hover {"
                "   background-color: %3;"
                "}")
            .arg(AppTheme::DARK_RED, AppTheme::BUTTON_WHITE_BG, "#600000"));
    m_mainLayout->addWidget(m_logoutButton, 0, Qt::AlignLeft);
    connect(m_logoutButton, &QPushButton::clicked, this, &ProfilePageWidget::onLogoutClicked);

    m_scrollArea->setWidget(m_contentWidget);
    pageLayout->addWidget(m_scrollArea);
}

void ProfilePageWidget::loadProfileData()
{
    m_statusMessageLabel->setVisible(false);
    if (!UserManager::instance()->isAuthenticated()) {
        m_displayNameHeaderLabel->setText("Not Logged In");
        m_emailHeaderLabel->setText("");

        m_editDisplayNameLineEdit->clear();
        m_editDisplayNameLineEdit->setEnabled(false);

        m_saveDisplayNameButton->setEnabled(false);

        m_editEmailLineEdit->clear();
        m_editEmailLineEdit->setEnabled(false);

        m_saveEmailButton->setEnabled(false);

        m_currentPasswordLineEdit->clear();
        m_currentPasswordLineEdit->setEnabled(false);

        m_newPasswordLineEdit->clear();
        m_newPasswordLineEdit->setEnabled(false);

        m_confirmNewPasswordLineEdit->clear();
        m_confirmNewPasswordLineEdit->setEnabled(false);

        m_changePasswordButton->setEnabled(false);

        m_profilePictureLabel->setText("Login");

        setProfilePicture(QPixmap());
        m_logoutButton->setText("Go to Login");
        return;
    }
    m_logoutButton->setText("Logout");

    m_initialDisplayName = UserManager::instance()->getDisplayName();
    m_initialEmail = UserManager::instance()->getEmail();
    bool isGoogleUser = UserManager::instance()->isGoogleSignIn();

    m_displayNameHeaderLabel->setText(m_initialDisplayName.isEmpty() ? "Set a Display Name"
                                                                     : m_initialDisplayName);
    m_emailHeaderLabel->setText(m_initialEmail);

    m_editDisplayNameLineEdit->setText(m_initialDisplayName);
    m_editDisplayNameLineEdit->setEnabled(true);
    m_saveDisplayNameButton->setEnabled(true);

    m_editEmailLineEdit->setText(m_initialEmail);
    m_editEmailLineEdit->setEnabled(!isGoogleUser);

    m_saveEmailButton->setEnabled(!isGoogleUser);
    if (isGoogleUser) {
        m_editEmailLineEdit->setPlaceholderText("Email managed by Google");
    } else {
        m_editEmailLineEdit->setPlaceholderText("New Email Address");
    }

    m_currentPasswordLineEdit->setEnabled(!isGoogleUser);
    m_newPasswordLineEdit->setEnabled(!isGoogleUser);
    m_confirmNewPasswordLineEdit->setEnabled(!isGoogleUser);
    m_changePasswordButton->setEnabled(!isGoogleUser);
    if (isGoogleUser) {
        m_currentPasswordLineEdit->setPlaceholderText("Password managed by Google");
        m_newPasswordLineEdit->setPlaceholderText("N/A for Google Sign-In");
        m_confirmNewPasswordLineEdit->setPlaceholderText("N/A for Google Sign-In");
    } else {
        m_currentPasswordLineEdit->setPlaceholderText("Current Password (for your reference)");
        m_newPasswordLineEdit->setPlaceholderText("New Password (min. 6 characters)");
        m_confirmNewPasswordLineEdit->setPlaceholderText("Confirm New Password");
    }

    m_currentLocalPicPath = UserManager::instance()->getProfilePictureUrl();
    loadProfilePictureFromLocalPath(m_currentLocalPicPath);

    qDebug() << "ProfilePageWidget: Loaded data for" << m_initialEmail
             << "Google User:" << isGoogleUser;
}

void ProfilePageWidget::updateUIFromUserManager()
{
    if (!UserManager::instance()->isAuthenticated())
        return;

    QString newDisplayName = UserManager::instance()->getDisplayName();
    if (m_displayNameHeaderLabel->text() != newDisplayName) {
        m_displayNameHeaderLabel->setText(newDisplayName.isEmpty() ? "Set a Display Name"
                                                                   : newDisplayName);
    }
    if (m_editDisplayNameLineEdit->text() != newDisplayName) {
        m_editDisplayNameLineEdit->setText(newDisplayName);
    }
    m_initialDisplayName = newDisplayName;

    QString newEmail = UserManager::instance()->getEmail();
    if (m_emailHeaderLabel->text() != newEmail) {
        m_emailHeaderLabel->setText(newEmail);
    }
    if (m_editEmailLineEdit->text() != newEmail && !UserManager::instance()->isGoogleSignIn()) {
        m_editEmailLineEdit->setText(newEmail);
    }
    m_initialEmail = newEmail;

    QString localPicPath = UserManager::instance()->getProfilePictureUrl();
    if (m_currentLocalPicPath != localPicPath) {
        loadProfilePictureFromLocalPath(localPicPath);
        m_currentLocalPicPath = localPicPath;
    }
}

void ProfilePageWidget::onProfilePictureClicked()
{
    if (!UserManager::instance()->isAuthenticated())
        return;

    QString selectedFilePath = QFileDialog::getOpenFileName(this,
                                                            "Select Profile Picture",
                                                            QDir::homePath(),
                                                            "Images (*.png *.jpg *.jpeg)");
    if (!selectedFilePath.isEmpty()) {
        QPixmap pixmap(selectedFilePath);
        if (!pixmap.isNull()) {
            QString uid = UserManager::instance()->getUid();
            if (uid.isEmpty()) {
                m_statusMessageLabel->setText("Error: User not identified. Cannot save picture.");
                m_statusMessageLabel->setStyleSheet(
                    QString("color: %1; font-size: 14px;").arg(AppTheme::PRIMARY_RED));
                m_statusMessageLabel->setVisible(true);
                return;
            }
            QString storageDir = getProfilePicStoragePath();
            QFileInfo fileInfo(selectedFilePath);
            QString newFileName = uid + "." + fileInfo.suffix().toLower();
            QString newFilePath = storageDir + "/" + newFileName;

            QStringList commonExtensions = {"png", "jpg", "jpeg"};
            for (const QString &ext : commonExtensions) {
                QString oldPossiblePath = storageDir + "/" + uid + "." + ext;
                if (oldPossiblePath != newFilePath && QFile::exists(oldPossiblePath)) {
                    QFile::remove(oldPossiblePath);
                }
            }

            if (QFile::copy(selectedFilePath, newFilePath)) {
                setProfilePicture(pixmap);
                m_currentLocalPicPath = newFilePath;

                if (m_firestoreService) {
                    QVariantMap fieldsToUpdate;
                    fieldsToUpdate["profilePictureLocalPath"] = newFilePath;
                    m_firestoreService->updateUserDocument(uid,
                                                           UserManager::instance()->getIdToken(),
                                                           fieldsToUpdate);
                    m_statusMessageLabel->setText("Updating profile picture...");
                    m_statusMessageLabel->setStyleSheet(
                        QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
                    m_statusMessageLabel->setVisible(true);
                } else {
                    UserManager::instance()->setProfilePictureUrl(newFilePath);
                    m_statusMessageLabel->setText("Profile picture updated locally.");
                    m_statusMessageLabel->setStyleSheet(
                        QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
                    m_statusMessageLabel->setVisible(true);
                }
            } else {
                m_statusMessageLabel->setText("Failed to save profile picture locally.");
                m_statusMessageLabel->setStyleSheet(
                    QString("color: %1; font-size: 14px;").arg(AppTheme::PRIMARY_RED));
                m_statusMessageLabel->setVisible(true);
                qWarning() << "Failed to copy profile picture from" << selectedFilePath << "to"
                           << newFilePath << "Error:" << QFile(newFilePath).errorString();
            }
        } else {
            m_statusMessageLabel->setText("Failed to load selected image.");
            m_statusMessageLabel->setStyleSheet(
                QString("color: %1; font-size: 14px;").arg(AppTheme::PRIMARY_RED));
            m_statusMessageLabel->setVisible(true);
        }
    }
}

void ProfilePageWidget::setProfilePicture(const QPixmap &pixmap)
{
    if (!m_profilePictureLabel)
        return;
    int diameter = m_profilePictureLabel->width();
    if (pixmap.isNull()) {
        m_profilePictureLabel->setText("No Pic");
        m_profilePictureLabel->setPixmap(QPixmap());
        m_profilePictureLabel->setStyleSheet(m_profilePictureLabel->styleSheet());
    } else {
        QPixmap finalPixmap(diameter, diameter);
        finalPixmap.fill(Qt::transparent);
        QPainter painter(&finalPixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QPainterPath path;
        path.addEllipse(0, 0, diameter, diameter);
        painter.setClipPath(path);
        QPixmap scaled = pixmap.scaled(diameter,
                                       diameter,
                                       Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation);
        int x = (diameter - scaled.width()) / 2;
        int y = (diameter - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
        m_profilePictureLabel->setText("");
        m_profilePictureLabel->setPixmap(finalPixmap);
    }
}

void ProfilePageWidget::loadProfilePictureFromLocalPath(const QString &localPath)
{
    if (localPath.isEmpty() || !QFile::exists(localPath)) {
        setProfilePicture(QPixmap());
        if (!localPath.isEmpty()) {
            qDebug() << "Profile picture not found at local path:" << localPath;
        }
        return;
    }
    QPixmap pixmap(localPath);
    if (!pixmap.isNull()) {
        setProfilePicture(pixmap);
    } else {
        qWarning() << "Failed to load profile picture from local path:" << localPath;
        setProfilePicture(QPixmap());
    }
}

void ProfilePageWidget::onSaveDisplayNameClicked()
{
    if (!m_authService || !UserManager::instance()->isAuthenticated()) {
        m_statusMessageLabel->setText("Service not available or not authenticated.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    QString newName = m_editDisplayNameLineEdit->text().trimmed();
    if (newName.isEmpty()) {
        m_statusMessageLabel->setText("Display name cannot be empty.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    if (newName == m_initialDisplayName) {
        m_statusMessageLabel->setText("Display name is already set to this value.");
        m_statusMessageLabel->setStyleSheet(
            QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    m_statusMessageLabel->setText("Updating display name...");
    m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_statusMessageLabel->setVisible(true);
    m_saveDisplayNameButton->setEnabled(false);

    m_authService->updateUserProfile(UserManager::instance()->getIdToken(), newName);
}

void ProfilePageWidget::onSaveEmailClicked()
{
    if (!m_authService || !UserManager::instance()->isAuthenticated()) {
        m_statusMessageLabel->setText("Service not available or not authenticated.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    if (UserManager::instance()->isGoogleSignIn()) {
        m_statusMessageLabel->setText("Email cannot be changed for Google Sign-In accounts.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    QString newEmail = m_editEmailLineEdit->text().trimmed();

    if (newEmail.isEmpty() || !newEmail.contains('@') || !newEmail.contains('.')) {
        m_statusMessageLabel->setText("Please enter a valid new email address.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    if (newEmail == m_initialEmail) {
        m_statusMessageLabel->setText("This is already your current email address.");
        m_statusMessageLabel->setStyleSheet(
            QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    m_statusMessageLabel->setText("Updating email address...");
    m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_statusMessageLabel->setVisible(true);
    m_saveEmailButton->setEnabled(false);

    // Pass empty string for currentPassword as it's not used by the REST API call here.
    m_authService->updateUserEmail(UserManager::instance()->getIdToken(), newEmail, "");
}

void ProfilePageWidget::onChangePasswordClicked()
{
    if (!m_authService || !UserManager::instance()->isAuthenticated()) {
        m_statusMessageLabel->setText("Service not available or not authenticated.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    if (UserManager::instance()->isGoogleSignIn()) {
        m_statusMessageLabel->setText("Password cannot be changed for Google Sign-In accounts.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    QString newPassword = m_newPasswordLineEdit->text();
    QString confirmPassword = m_confirmNewPasswordLineEdit->text();

    if (newPassword.isEmpty() || confirmPassword.isEmpty()) {
        m_statusMessageLabel->setText("Please fill in new password fields.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    if (newPassword.length() < 6) {
        m_statusMessageLabel->setText("New password must be at least 6 characters long.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }
    if (newPassword != confirmPassword) {
        m_statusMessageLabel->setText("New passwords do not match.");
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
        return;
    }

    m_statusMessageLabel->setText("Changing password...");
    m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
    m_statusMessageLabel->setVisible(true);
    m_changePasswordButton->setEnabled(false);

    m_authService->updateUserPassword(UserManager::instance()->getIdToken(), newPassword);
}

void ProfilePageWidget::handleProfileAuthUpdateSuccess(const QString &operation,
                                                       const QJsonObject &responseData)
{
    qDebug() << "ProfilePageWidget: Auth Update Success for" << operation << responseData;
    m_statusMessageLabel->setVisible(true);
    QString successMessage;
    bool needsFirestoreUpdate = false;
    QVariantMap firestoreFieldsToUpdate;

    QString newIdToken = responseData.value("idToken").toString();
    if (!newIdToken.isEmpty()) {
        UserManager::instance()->updateIdToken(newIdToken);
    }

    if (operation == "UpdateUserProfile") { // Display Name
        QString newDisplayNameFromAuth = responseData.value("displayName").toString();
        successMessage = "Display name updated successfully.";
        m_saveDisplayNameButton->setEnabled(true);

        firestoreFieldsToUpdate["displayName"] = newDisplayNameFromAuth;
        needsFirestoreUpdate = true;

    } else if (operation == "UpdateUserEmail") {
        QString newEmailFromAuth = responseData.value("email").toString();
        successMessage
            = "Email update request successful. Check your new email for verification if needed.";
        m_saveEmailButton->setEnabled(true);
        m_currentPasswordForEmailLineEdit->clear();

        firestoreFieldsToUpdate["email"] = newEmailFromAuth;
        needsFirestoreUpdate = true;

    } else if (operation == "UpdateUserPassword") {
        successMessage = "Password changed successfully.";
        m_changePasswordButton->setEnabled(true);
        m_currentPasswordLineEdit->clear();
        m_newPasswordLineEdit->clear();
        m_confirmNewPasswordLineEdit->clear();
    } else {
        successMessage = "Profile updated successfully.";
    }

    m_statusMessageLabel->setText(successMessage);
    m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));

    if (needsFirestoreUpdate && m_firestoreService && UserManager::instance()->isAuthenticated()) {
        m_firestoreService->updateUserDocument(UserManager::instance()->getUid(),
                                               UserManager::instance()->getIdToken(),
                                               firestoreFieldsToUpdate);
    } else if (needsFirestoreUpdate) {
        if (firestoreFieldsToUpdate.contains("displayName")) {
            UserManager::instance()->setDisplayName(
                firestoreFieldsToUpdate["displayName"].toString());
        }
        if (firestoreFieldsToUpdate.contains("email")) {
            UserManager::instance()->setEmail(firestoreFieldsToUpdate["email"].toString());
        }
    }
}

void ProfilePageWidget::handleProfileAuthUpdateFailed(const QString &operation, const QString &error)
{
    qWarning() << "ProfilePageWidget: Auth Update Failed for" << operation << ":" << error;
    m_statusMessageLabel->setVisible(true);

    m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));

    if (operation == "UpdateUserProfile")
        m_saveDisplayNameButton->setEnabled(true);
    else if (operation == "UpdateUserEmail")
        m_saveEmailButton->setEnabled(true);
    else if (operation == "UpdateUserPassword")
        m_changePasswordButton->setEnabled(true);
}

void ProfilePageWidget::handleFirestoreUpdateSuccess(bool success,
                                                     const QString &uid,
                                                     const QJsonObject &updatedFields,
                                                     const QString &error)
{
    Q_UNUSED(uid);
    if (success) {
        if (m_statusMessageLabel->text().contains("Updating...")) {
            m_statusMessageLabel->setText("Profile details saved successfully.");
            m_statusMessageLabel->setStyleSheet(
                QString("color: %1;").arg(AppTheme::TEXT_ON_DARK_SECONDARY));
        }
        m_statusMessageLabel->setVisible(true);

        if (updatedFields.contains("displayName")) {
            UserManager::instance()->setDisplayName(updatedFields["displayName"].toString());
        }
        if (updatedFields.contains("email")) {
            UserManager::instance()->setEmail(updatedFields["email"].toString());
        }
        if (updatedFields.contains("profilePictureLocalPath")) {
            UserManager::instance()->setProfilePictureUrl(
                updatedFields["profilePictureLocalPath"].toString());
        }
    } else {
        m_statusMessageLabel->setText("Failed to save some profile details to database: " + error);
        m_statusMessageLabel->setStyleSheet(QString("color: %1;").arg(AppTheme::PRIMARY_RED));
        m_statusMessageLabel->setVisible(true);
    }
}

void ProfilePageWidget::onLogoutClicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,
                                  "Logout",
                                  "Are you sure you want to logout?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit logoutRequested();
    }
}
