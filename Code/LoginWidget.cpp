#include "LoginWidget.h"
#include "FirestoreService.h"
#include "Theme.h"
#include "UserManager.h"
#include "ConfigManager.h"
#include <QApplication>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QPainter>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QTimer>

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

InputField::InputField(const QString &iconPath,
                       const QString &placeholder,
                       bool isPassword,
                       QWidget *parent)
    : QWidget(parent)
{
    setObjectName("inputField");
    setMinimumHeight(48);
    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("inputIcon");
    QIcon icon(iconPath);
    if (!icon.isNull()) {
        int iconDisplaySize = 20;
        m_iconLabel->setPixmap(icon.pixmap(QSize(iconDisplaySize, iconDisplaySize)));
        m_iconLabel->setFixedSize(iconDisplaySize, iconDisplaySize);
    } else {
        m_iconLabel->setText("?");
        m_iconLabel->setFixedSize(18, 18);
        qWarning() << "InputField: Icon not found or invalid at path:" << iconPath;
    }
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText(placeholder);
    m_lineEdit->setFrame(false);
    m_lineEdit->setObjectName("inputText");
    if (isPassword) {
        m_lineEdit->setEchoMode(QLineEdit::Password);
    }
    m_lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 5, 0);
    layout->setSpacing(8);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_lineEdit);
    setLayout(layout);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &InputField::textChanged);
}

QString InputField::text() const
{
    return m_lineEdit->text();
}

void InputField::setText(const QString &text)
{
    m_lineEdit->setText(text);
}

void InputField::clear()
{
    m_lineEdit->clear();
}

void InputField::setFocus()
{
    m_lineEdit->setFocus();
}

RegistrationForm::RegistrationForm(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("formWidget");
    setupUI();
}
void RegistrationForm::setupUI()
{
    QVBoxLayout *formLayout = new QVBoxLayout(this);
    formLayout->setContentsMargins(40, 20, 40, 20);
    formLayout->setSpacing(15);
    formLayout->addStretch(1);
    QLabel *titleLabel = new QLabel("Create Account", this);
    titleLabel->setObjectName("formTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(titleLabel);
    QLabel *separatorLabel = new QLabel("Please provide your details:", this);
    separatorLabel->setObjectName("formSeparator");
    separatorLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(separatorLabel);
    nameInput = new InputField(":/assets/icons/user_icon.svg", "Full Name", false, this);
    formLayout->addWidget(nameInput);
    emailInput = new InputField(":/assets/icons/email_icon.svg", "Email Address", false, this);
    formLayout->addWidget(emailInput);
    passwordInput = new InputField(":/assets/icons/lock_icon.svg",
                                   "Password (min 6 chars)",
                                   true,
                                   this);
    formLayout->addWidget(passwordInput);
    errorLabel = new QLabel("", this);
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    formLayout->addWidget(errorLabel);
    registerButton = new QPushButton("SIGN UP", this);
    registerButton->setObjectName("formActionButton");
    registerButton->setMinimumHeight(45);
    registerButton->setCursor(Qt::PointingHandCursor);
    registerButton->setEnabled(false);
    formLayout->addWidget(registerButton);
    QLabel *orSeparatorLabel = new QLabel("──────── OR ────────", this);
    orSeparatorLabel->setObjectName("orSeparator");
    orSeparatorLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(orSeparatorLabel);
    googleButton_reg = createGoogleButton("googleButton_reg");
    formLayout->addWidget(googleButton_reg, 0, Qt::AlignCenter);
    formLayout->addStretch(1);
    connect(registerButton, &QPushButton::clicked, this, &RegistrationForm::onRegisterClicked);
    connect(nameInput, &InputField::textChanged, this, &RegistrationForm::onInputChanged);
    connect(emailInput, &InputField::textChanged, this, &RegistrationForm::onInputChanged);
    connect(passwordInput, &InputField::textChanged, this, &RegistrationForm::onInputChanged);
    connect(googleButton_reg, &QPushButton::clicked, this, &RegistrationForm::onGoogleClicked_reg);
}
QPushButton *RegistrationForm::createGoogleButton(const QString &objectName)
{
    QPushButton *btn = new QPushButton("Sign Up with Google", this);
    btn->setObjectName(objectName);
    btn->setMinimumHeight(45);
    btn->setIcon(QIcon(":/assets/icons/google_icon.svg"));
    btn->setIconSize(QSize(20, 20));
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}
void RegistrationForm::onRegisterClicked()
{
    QString name = nameInput->text().trimmed();
    QString email = emailInput->text().trimmed();
    QString password = passwordInput->text();
    errorLabel->setVisible(false);
    if (name.isEmpty() || email.isEmpty() || password.isEmpty()) {
        errorLabel->setText("Please fill in all fields.");
        errorLabel->setVisible(true);
        return;
    }
    if (!email.contains('@') || !email.contains('.')) {
        errorLabel->setText("Please enter a valid email.");
        errorLabel->setVisible(true);
        return;
    }
    if (password.length() < 6) {
        errorLabel->setText("Password must be at least 6 characters.");
        errorLabel->setVisible(true);
        return;
    }
    emit registrationAttempted(name, email, password);
}

void RegistrationForm::onInputChanged()
{
    emit validateFormSignal();
}

void RegistrationForm::clearForm()
{
    nameInput->clear();
    emailInput->clear();
    passwordInput->clear();
    errorLabel->setVisible(false);
    onInputChanged();
}

void RegistrationForm::onGoogleClicked_reg()
{
    emit googleSignInRequested_reg();
}

QString RegistrationForm::getFullName() const
{
    return nameInput->text().trimmed();
}

LoginForm::LoginForm(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("formWidget");
    setupUI();
}
void LoginForm::setupUI()
{
    QVBoxLayout *formLayout = new QVBoxLayout(this);
    formLayout->setContentsMargins(40, 20, 40, 20);
    formLayout->setSpacing(15);
    formLayout->addStretch(1);
    QLabel *titleLabel = new QLabel("Sign in to Cinephile", this);
    titleLabel->setObjectName("formTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(titleLabel);
    QLabel *separatorLabel = new QLabel("Enter your credentials:", this);
    separatorLabel->setObjectName("formSeparator");
    separatorLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(separatorLabel);
    emailInput = new InputField(":/assets/icons/email_icon.svg", "Email Address", false, this);
    formLayout->addWidget(emailInput);
    passwordInput = new InputField(":/assets/icons/lock_icon.svg", "Password", true, this);
    formLayout->addWidget(passwordInput);
    errorLabel = new QLabel("", this);
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    formLayout->addWidget(errorLabel);
    loginButton = new QPushButton("SIGN IN", this);
    loginButton->setObjectName("formActionButton");
    loginButton->setMinimumHeight(45);
    loginButton->setCursor(Qt::PointingHandCursor);
    loginButton->setEnabled(false);
    formLayout->addWidget(loginButton);
    QLabel *orSeparatorLabel = new QLabel("──────── OR ────────", this);
    orSeparatorLabel->setObjectName("orSeparator");
    orSeparatorLabel->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(orSeparatorLabel);
    googleButton_log = createGoogleButton("googleButton_log");
    formLayout->addWidget(googleButton_log, 0, Qt::AlignCenter);
    formLayout->addStretch(1);
    connect(loginButton, &QPushButton::clicked, this, &LoginForm::onLoginClicked);
    connect(emailInput, &InputField::textChanged, this, &LoginForm::onInputChanged);
    connect(passwordInput, &InputField::textChanged, this, &LoginForm::onInputChanged);
    connect(googleButton_log, &QPushButton::clicked, this, &LoginForm::onGoogleClicked_log);
}
QPushButton *LoginForm::createGoogleButton(const QString &objectName)
{
    QPushButton *btn = new QPushButton("Sign In with Google", this);
    btn->setObjectName(objectName);
    btn->setMinimumHeight(45);
    btn->setIcon(QIcon(":/assets/icons/google_icon.svg"));
    btn->setIconSize(QSize(20, 20));
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}
void LoginForm::onLoginClicked()
{
    QString email = emailInput->text().trimmed();
    QString password = passwordInput->text();
    errorLabel->setVisible(false);
    if (email.isEmpty() || password.isEmpty()) {
        errorLabel->setText("Please enter email and password.");
        errorLabel->setVisible(true);
        return;
    }
    if (!email.contains('@') || !email.contains('.')) {
        errorLabel->setText("Please enter a valid email.");
        errorLabel->setVisible(true);
        return;
    }
    emit loginAttempted(email, password);
}
void LoginForm::onGoogleClicked_log()
{
    emit googleSignInRequested_log();
}

void LoginForm::onInputChanged()
{
    emit validateFormSignal();
}

void LoginForm::clearForm()
{
    emailInput->clear();
    passwordInput->clear();
    errorLabel->setVisible(false);
    onInputChanged();
}

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_firebaseApiKey(ConfigManager::instance()->getFirebaseApiKey())
{


    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("loginWidgetBase");
    setupUI();
    applyStyles();
    setupAnimations();
    isLoginVisible = true;
    m_initialPositionsSet = false;

    m_firestoreService = new FirestoreService(m_firebaseApiKey, this);
    UserManager::instance()->setApiKey(m_firebaseApiKey);

    connect(m_networkManager,
            &QNetworkAccessManager::finished,
            this,
            &LoginWidget::onAuthReplyFinished);
    connect(loginForm, &LoginForm::loginAttempted, this, &LoginWidget::handleLoginAttemptFromForm);
    connect(registrationForm,
            &RegistrationForm::registrationAttempted,
            this,
            &LoginWidget::handleRegistrationAttemptFromForm);
    connect(loginForm, &LoginForm::validateFormSignal, this, &LoginWidget::onLoginValidate);
    connect(registrationForm,
            &RegistrationForm::validateFormSignal,
            this,
            &LoginWidget::onRegisterValidate);
    connect(loginForm,
            &LoginForm::googleSignInRequested_log,
            this,
            &LoginWidget::handleGoogleSignInRequested);
    connect(registrationForm,
            &RegistrationForm::googleSignInRequested_reg,
            this,
            &LoginWidget::handleGoogleSignInRequested);

    connect(m_firestoreService,
            &FirestoreService::userProfileCheckCreateComplete,
            this,
            &LoginWidget::onFirestoreUserProfileChecked);
}

LoginWidget::~LoginWidget()
{
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void LoginWidget::updateUserProfile(const QString &idToken,
                                    const QString &displayName,
                                    const QString &photoUrl)
{
    if (m_firebaseApiKey.isEmpty() || m_firebaseApiKey.contains("PLACEHOLDER")) {
        emit profileUpdateFailed("UpdateUserProfile", "Firebase API Key not configured.");
        return;
    }
    m_currentAuthOperation = AuthOperation::UpdateUserProfile;
    m_currentProfileUpdateOperationName = "UpdateUserProfile";

    QJsonObject payload;
    payload["idToken"] = idToken;
    if (!displayName.isEmpty()) {
        payload["displayName"] = displayName;
    }
    if (!photoUrl.isEmpty()) {
        payload["photoUrl"] = photoUrl;
    } else {
        payload["photoUrl"] = "";
    }
    payload["returnSecureToken"] = true;

    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();

    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:update?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->post(request, jsonData);
}

void LoginWidget::updateUserEmail(const QString &idToken,
                                  const QString &newEmail,
                                  const QString &currentPassword)
{
    if (m_firebaseApiKey.isEmpty() || m_firebaseApiKey.contains("PLACEHOLDER")) {
        emit profileUpdateFailed("UpdateUserEmail", "Firebase API Key not configured.");
        return;
    }
    m_currentAuthOperation = AuthOperation::UpdateEmail;
    m_currentProfileUpdateOperationName = "UpdateUserEmail";

    QJsonObject payload;
    payload["idToken"] = idToken;
    payload["email"] = newEmail;
    payload["returnSecureToken"] = true;

    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();

    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:update?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->post(request, jsonData);
}

void LoginWidget::updateUserPassword(const QString &idToken, const QString &newPassword)
{
    if (m_firebaseApiKey.isEmpty() || m_firebaseApiKey.contains("PLACEHOLDER")) {
        emit profileUpdateFailed("UpdateUserPassword", "Firebase API Key not configured.");
        return;
    }
    m_currentAuthOperation = AuthOperation::UpdatePassword;
    m_currentProfileUpdateOperationName = "UpdateUserPassword";

    QJsonObject payload;
    payload["idToken"] = idToken;
    payload["password"] = newPassword;
    payload["returnSecureToken"] = true;

    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();

    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:update?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->post(request, jsonData);
}

void LoginWidget::handleRegistrationAttemptFromForm(const QString &name,
                                                    const QString &email,
                                                    const QString &password)
{
    Q_UNUSED(name);
    if (m_firebaseApiKey.isEmpty() || m_firebaseApiKey.contains("PLACEHOLDER")) {
        showRegistrationError("Firebase API Key not configured correctly.");
        qCritical() << "Firebase API Key not configured in LoginWidget or is placeholder.";
        return;
    }
    registrationForm->registerButton->setEnabled(false);
    registrationForm->errorLabel->setText("Registering...");
    registrationForm->errorLabel->setVisible(true);
    m_currentAuthOperation = AuthOperation::SignUp;
    m_pendingAuthDataContext.isGoogleSignIn = false;

    m_pendingAuthDataContext.displayNameFromAuth = name;

    QJsonObject jsonPayload;
    jsonPayload["email"] = email;
    jsonPayload["password"] = password;
    jsonPayload["returnSecureToken"] = true;
    QJsonDocument jsonDoc(jsonPayload);
    QByteArray jsonData = jsonDoc.toJson();
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->post(request, jsonData);
}

void LoginWidget::handleLoginAttemptFromForm(const QString &email, const QString &password)
{
    if (m_firebaseApiKey.isEmpty() || m_firebaseApiKey.contains("PLACEHOLDER")) {
        showLoginError("Firebase API Key not configured correctly.");
        qCritical() << "Firebase API Key not configured in LoginWidget or is placeholder.";
        return;
    }
    loginForm->loginButton->setEnabled(false);
    loginForm->errorLabel->setText("Signing in...");
    loginForm->errorLabel->setVisible(true);
    m_currentAuthOperation = AuthOperation::SignIn;
    m_pendingAuthDataContext.isGoogleSignIn = false;
    m_pendingAuthDataContext.displayNameFromAuth.clear();

    QJsonObject jsonPayload;
    jsonPayload["email"] = email;
    jsonPayload["password"] = password;
    jsonPayload["returnSecureToken"] = true;
    QJsonDocument jsonDoc(jsonPayload);
    QByteArray jsonData = jsonDoc.toJson();
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->post(request, jsonData);
}

void LoginWidget::onAuthReplyFinished(QNetworkReply *reply)
{
    if (!reply) {
        qWarning() << "onAuthReplyFinished: Received a null reply.";
        if (m_currentAuthOperation == AuthOperation::SignUp && registrationForm)
            registrationForm->registerButton->setEnabled(true);

        if (m_currentAuthOperation == AuthOperation::SignIn && loginForm)
            loginForm->loginButton->setEnabled(true);

        if (m_currentAuthOperation == AuthOperation::UpdateUserProfile
            || m_currentAuthOperation == AuthOperation::UpdateEmail
            || m_currentAuthOperation == AuthOperation::UpdatePassword) {
            emit profileUpdateFailed(m_currentProfileUpdateOperationName, "Null reply from server.");
        }

        m_currentAuthOperation = AuthOperation::None;
        m_currentProfileUpdateOperationName.clear();
        return;
    }

    if (reply->request().url().host() == "oauth2.googleapis.com"
        && reply->request().url().path() == "/token") {
        qDebug() << "onAuthReplyFinished: Skipping Google Token Exchange reply (URL:"
                 << reply->request().url().toString()
                 << "), as it's handled by onGoogleTokenReplyFinished.";
        return;
    }

    AuthOperation operationCompleted = m_currentAuthOperation;
    QString profileUpdateOpName = m_currentProfileUpdateOperationName;

    if (operationCompleted == AuthOperation::SignUp && registrationForm) {
        registrationForm->registerButton->setEnabled(true);
    } else if (operationCompleted == AuthOperation::SignIn && loginForm) {
        loginForm->loginButton->setEnabled(true);
    }

    QByteArray responseData = reply->readAll();
    QNetworkReply::NetworkError networkError = reply->error();
    QString replyErrorString = reply->errorString();
    reply->deleteLater();

    QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(responseData);
    if (jsonResponseDoc.isNull() || !jsonResponseDoc.isObject()) {
        QString errorMsg = "Invalid JSON response from Firebase server.";
        if (operationCompleted == AuthOperation::SignUp)
            showRegistrationError(errorMsg);
        else if (operationCompleted == AuthOperation::SignIn
                 || operationCompleted == AuthOperation::GoogleSignInIdP)
            showLoginError(errorMsg);
        else if (operationCompleted == AuthOperation::UpdateUserProfile
                 || operationCompleted == AuthOperation::UpdateEmail
                 || operationCompleted == AuthOperation::UpdatePassword) {
            emit profileUpdateFailed(profileUpdateOpName, errorMsg);
        }
        qWarning() << "Firebase Auth Reply Error: Invalid JSON -" << responseData
                   << "for operation:" << static_cast<int>(operationCompleted);
        m_currentAuthOperation = AuthOperation::None;
        m_currentProfileUpdateOperationName.clear();
        return;
    }

    QJsonObject jsonResponse = jsonResponseDoc.object();
    qDebug() << "Firebase Auth Response (Op: " << static_cast<int>(operationCompleted)
             << "):" << jsonResponse.keys();

    if (networkError == QNetworkReply::NoError) {
        if (jsonResponse.contains("error")) {
            QJsonObject errorObj = jsonResponse["error"].toObject();
            QString message = errorObj["message"].toString("Unknown error from Firebase.");
            qWarning() << "Firebase Auth Error (in JSON response):" << message;
            if (operationCompleted == AuthOperation::SignUp)
                showRegistrationError(message);
            else if (operationCompleted == AuthOperation::SignIn
                     || operationCompleted == AuthOperation::GoogleSignInIdP)
                showLoginError(message);
            else if (operationCompleted == AuthOperation::UpdateUserProfile
                     || operationCompleted == AuthOperation::UpdateEmail
                     || operationCompleted == AuthOperation::UpdatePassword) {
                emit profileUpdateFailed(profileUpdateOpName, message);
            }
            m_currentAuthOperation = AuthOperation::None;
            m_currentProfileUpdateOperationName.clear();
            return;
        }

        if (operationCompleted == AuthOperation::SignUp
            || operationCompleted == AuthOperation::SignIn
            || operationCompleted == AuthOperation::GoogleSignInIdP) {
            QString idToken = jsonResponse["idToken"].toString();
            QString refreshToken = jsonResponse["refreshToken"].toString();
            QString localId = jsonResponse["localId"].toString();
            QString emailVal = jsonResponse["email"].toString();
            QString displayNameFromAuthResponse = jsonResponse.value("displayName").toString();

            if (idToken.isEmpty() || localId.isEmpty()) {
                QString errorMsg = "Firebase Auth success but missing token or UID.";
                if (operationCompleted == AuthOperation::SignUp)
                    showRegistrationError(errorMsg);
                else
                    showLoginError(errorMsg);
                m_currentAuthOperation = AuthOperation::None;
                return;
            }

            m_pendingAuthDataContext.uid = localId;
            m_pendingAuthDataContext.idToken = idToken;
            m_pendingAuthDataContext.refreshToken = refreshToken;
            m_pendingAuthDataContext.email = emailVal;
            if (!displayNameFromAuthResponse.isEmpty()) {
                m_pendingAuthDataContext.displayNameFromAuth = displayNameFromAuthResponse;
            }

            m_currentAuthOperation = AuthOperation::FirestoreProfileCheck;
            if (isLoginVisible && loginForm) {
                loginForm->errorLabel->setText("Finalizing login...");
                loginForm->errorLabel->setVisible(true);
            } else if (registrationForm) {
                registrationForm->errorLabel->setText("Finalizing registration...");
                registrationForm->errorLabel->setVisible(true);
            }

            m_firestoreService
                ->checkAndCreateUserProfile(localId,
                                            idToken,
                                            emailVal,
                                            m_pendingAuthDataContext.displayNameFromAuth);
        } else if (operationCompleted == AuthOperation::UpdateUserProfile
                   || operationCompleted == AuthOperation::UpdateEmail
                   || operationCompleted == AuthOperation::UpdatePassword) {

            QString newIdToken = jsonResponse.value("idToken").toString();
            if (!newIdToken.isEmpty()) {
                UserManager::instance()->updateIdToken(
                    newIdToken);
            }
            emit profileUpdateSuccess(profileUpdateOpName, jsonResponse);
            m_currentAuthOperation = AuthOperation::None;
            m_currentProfileUpdateOperationName.clear();
        }

    } else {
        QString errorString;
        if (jsonResponse.contains("error")) {
            QJsonObject errorObj = jsonResponse["error"].toObject();
            errorString = errorObj["message"].toString(
                QString("Network Error Code: %1. %2").arg(networkError).arg(replyErrorString));
        } else {
            errorString = QString("Network Error: %1. Response: %2")
                              .arg(replyErrorString)
                              .arg(QString::fromUtf8(responseData));
        }
        qWarning() << "Firebase Network Auth Error Details: " << errorString;
        if (operationCompleted == AuthOperation::SignUp)
            showRegistrationError(QString("Error: %1").arg(errorString));
        else if (operationCompleted == AuthOperation::SignIn
                 || operationCompleted == AuthOperation::GoogleSignInIdP)
            showLoginError(QString("Error: %1").arg(errorString));
        else if (operationCompleted == AuthOperation::UpdateUserProfile
                 || operationCompleted == AuthOperation::UpdateEmail
                 || operationCompleted == AuthOperation::UpdatePassword) {
            emit profileUpdateFailed(profileUpdateOpName, errorString);
        }
        m_currentAuthOperation = AuthOperation::None;
        m_currentProfileUpdateOperationName.clear();
    }
}

void LoginWidget::onFirestoreUserProfileChecked(bool success,
                                                const QString &uid,
                                                const QJsonObject &profileData,
                                                const QString &error)
{
    m_currentAuthOperation = AuthOperation::None;

    if (uid != m_pendingAuthDataContext.uid) {
        qWarning() << "Firestore callback UID mismatch!";
        showLoginError("Internal error during login finalization.");
        UserManager::instance()->clearCurrentUser();
        return;
    }

    if (success) {
        if (loginForm && isLoginVisible)
            loginForm->errorLabel->setVisible(false);
        if (registrationForm && !isLoginVisible)
            registrationForm->errorLabel->setVisible(false);

        QString finalDisplayName = profileData.value("displayName").toString();
        if (finalDisplayName
                .isEmpty()) {
            finalDisplayName = m_pendingAuthDataContext.displayNameFromAuth;
        }
        QString emailFromProfile = profileData.value("email").toString(
            m_pendingAuthDataContext.email);

        proceedToApp(m_pendingAuthDataContext.uid,
                     m_pendingAuthDataContext.idToken,
                     m_pendingAuthDataContext.refreshToken,
                     emailFromProfile,
                     finalDisplayName,
                     m_pendingAuthDataContext.isGoogleSignIn);
    } else {
        qWarning() << "Firestore User Profile Check/Create Failed for UID:" << uid
                   << "Error:" << error;
        QString userMessage = "Failed to finalize login: " + error;
        if (isLoginVisible && loginForm)
            showLoginError(userMessage);
        else if (registrationForm)
            showRegistrationError(userMessage);
        UserManager::instance()->clearCurrentUser();
    }

    m_pendingAuthDataContext = AuthDataContext();
}

void LoginWidget::proceedToApp(const QString &uid,
                               const QString &idToken,
                               const QString &refreshToken,
                               const QString &email,
                               const QString &displayName,
                               bool isGoogleSignIn)
{
    if (loginForm && isLoginVisible)
        loginForm->errorLabel->setVisible(false);
    if (registrationForm && !isLoginVisible)
        registrationForm->errorLabel->setVisible(false);

    UserManager::instance()
        ->setCurrentUser(uid, idToken, refreshToken, email, displayName, isGoogleSignIn);

    emit userAuthenticated(uid, idToken, refreshToken, email, displayName);
}

void LoginWidget::handleGoogleSignInRequested()
{
    if (isLoginVisible && loginForm) {
        loginForm->errorLabel->setText("Redirecting to Google...");
        loginForm->errorLabel->setVisible(true);
    } else if (registrationForm) {
        registrationForm->errorLabel->setText("Redirecting to Google...");
        registrationForm->errorLabel->setVisible(true);
    }

    m_pendingAuthDataContext.displayNameFromAuth.clear();
    m_pendingAuthDataContext.isGoogleSignIn = true;
    startGoogleOAuthFlow();
}

void LoginWidget::startGoogleOAuthFlow()
{
    if (!m_tcpServer) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &LoginWidget::handleNewTcpConnection);
    }
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    if (!m_tcpServer->listen(QHostAddress::LocalHost, 0)) {
        showLoginError(QString("Google Sign-In error: Could not start local server."));

        if (isLoginVisible && loginForm)
            loginForm->errorLabel->setVisible(true);

        else if (registrationForm)
            registrationForm->errorLabel->setVisible(true);

        return;
    }
    m_redirectPort = m_tcpServer->serverPort();
    qDebug() << "Local TCP server listening on port" << m_redirectPort;

    m_googleCodeVerifier = QUuid::createUuid().toString(QUuid::WithoutBraces)
                           + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_googleCodeVerifier = m_googleCodeVerifier.left(128);
    QByteArray codeChallengeBytes = QCryptographicHash::hash(m_googleCodeVerifier.toUtf8(),
                                                             QCryptographicHash::Sha256);
    QString codeChallenge = QString::fromUtf8(codeChallengeBytes.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    m_googleAuthState = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QUrlQuery query;
    query.addQueryItem("client_id", m_googleClientId);
    query.addQueryItem("redirect_uri", QString("http://localhost:%1").arg(m_redirectPort));
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", "openid email profile");
    query.addQueryItem("state", m_googleAuthState);
    query.addQueryItem("code_challenge", codeChallenge);
    query.addQueryItem("code_challenge_method", "S256");
    QUrl authUrl("https://accounts.google.com/o/oauth2/v2/auth");
    authUrl.setQuery(query);
    qDebug() << "Opening Google Auth URL:" << authUrl.toString();
    if (!QDesktopServices::openUrl(authUrl)) {
        qWarning() << "Failed to open URL in default browser.";
        showLoginError("Could not open browser for Google Sign-In.");
        if (m_tcpServer)
            m_tcpServer->close();
        if (isLoginVisible && loginForm)
            loginForm->errorLabel->setVisible(true);
        else if (registrationForm)
            registrationForm->errorLabel->setVisible(true);
    }
}

void LoginWidget::handleNewTcpConnection()
{
    if (!m_tcpServer)
        return;
    if (m_tcpSocket) {
        qWarning() << "Already handling a TCP connection for redirect.";
        QTcpSocket *extraSocket = m_tcpServer->nextPendingConnection();
        if (extraSocket) {
            extraSocket->abort();
            extraSocket->deleteLater();
        }
        return;
    }
    m_tcpSocket = m_tcpServer->nextPendingConnection();
    if (m_tcpSocket) {
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &LoginWidget::handleTcpSocketReadyRead);
        connect(m_tcpSocket,
                &QTcpSocket::disconnected,
                this,
                &LoginWidget::handleTcpSocketDisconnected);
        qDebug() << "Accepted new TCP connection for Google redirect.";
    }
}

void LoginWidget::handleTcpSocketReadyRead()
{
    if (!m_tcpSocket || !m_tcpSocket->canReadLine())
        return;
    QByteArray requestData = m_tcpSocket->readAll();
    QString requestStr = QString::fromUtf8(requestData);
    qDebug().noquote() << "Received from browser (full request might be multi-line):\n"
                       << requestStr.left(500);
    QStringList lines = requestStr.split("\r\n");
    QString requestLine;
    if (!lines.isEmpty())
        requestLine = lines.first();

    if (requestLine.startsWith("GET ")) {
        QStringList parts = requestLine.split(" ");
        if (parts.length() >= 2) {
            QUrl callbackUrl("http://localhost" + parts[1]);
            QUrlQuery query(callbackUrl.query());
            QString receivedState = query.queryItemValue("state");
            QString authCode = query.queryItemValue("code");
            QString error = query.queryItemValue("error");
            QString htmlTemplate = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%1</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Google Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background-color: #ffffff;
            color: #202124;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            line-height: 1.5;
        }

        .container {
            background: #ffffff;
            border-radius: 8px;
            padding: 48px 40px;
            text-align: center;
            max-width: 450px;
            width: 90%;
            box-shadow: 0 2px 10px 0 rgba(0,0,0,0.2);
            border: 1px solid #dadce0;
        }

        .logo-container {
            margin-bottom: 32px;
        }

        .google-logo {
            width: 92px;
            height: 92px;
            margin: 0 auto 24px;
            background: #ffffff;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }

        .google-logo svg {
            width: 48px;
            height: 48px;
        }

        .app-info {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
            margin-bottom: 32px;
        }

        .app-logo {
            width: 40px;
            height: 40px;
            background: #1a73e8;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: 500;
            font-size: 18px;
        }

        .arrow {
            color: #5f6368;
            font-size: 18px;
        }

        h1 {
            font-size: 24px;
            font-weight: 400;
            margin-bottom: 8px;
            color: #202124;
        }

        .subtitle {
            font-size: 16px;
            color: #5f6368;
            margin-bottom: 32px;
        }

        .success-message {
            background: #e8f5e8;
            border: 1px solid #c3e6c3;
            border-radius: 4px;
            padding: 16px;
            margin-bottom: 32px;
            color: #137333;
            font-size: 14px;
        }

        .error-message {
            background: #fce8e6;
            border: 1px solid #f9ab00;
            border-radius: 4px;
            padding: 16px;
            margin-bottom: 32px;
            color: #d93025;
            font-size: 14px;
            text-align: left;
        }

        .status-icon {
            width: 24px;
            height: 24px;
            margin: 0 auto 16px;
        }

        .success-icon {
            background: #137333;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 16px;
            font-weight: bold;
        }

        .error-icon {
            background: #d93025;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 16px;
            font-weight: bold;
        }

        .instructions {
            font-size: 14px;
            color: #5f6368;
            margin-bottom: 24px;
            line-height: 1.4;
        }

        .close-info {
            font-size: 12px;
            color: #80868b;
            padding-top: 24px;
            border-top: 1px solid #e8eaed;
        }

        .countdown {
            font-weight: 500;
            color: #1a73e8;
        }

        @media (max-width: 480px) {
            .container {
                padding: 32px 24px;
                margin: 16px;
            }

            h1 {
                font-size: 20px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo-container">
            <div class="google-logo">
                <svg viewBox="0 0 24 24">
                    <path fill="#4285f4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"/>
                    <path fill="#34a853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"/>
                    <path fill="#fbbc05" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"/>
                    <path fill="#ea4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"/>
                </svg>
            </div>
            <div class="app-info">
                <div class="app-logo">C</div>
                <span class="arrow">←</span>
                <span style="font-size: 14px; color: #5f6368;">Google</span>
            </div>
        </div>

        %2

        <h1>%3</h1>
        <div class="subtitle">%4</div>

        <div class="instructions">
            You can now close this tab and return to Cinephile to continue. 😊
        </div>

        <div class="close-info">
            This tab will close automatically in <span class="countdown" id="countdown">%5</span> seconds 👋
        </div>
    </div>

    <script>
        let timeLeft = %5;
        const countdownElement = document.getElementById('countdown');

        const timer = setInterval(() => {
            timeLeft--;
            countdownElement.textContent = timeLeft;

            if (timeLeft <= 0) {
                clearInterval(timer);
                window.close();
            }
        }, 1000);

        // Allow manual close
        document.addEventListener('click', () => {
            window.close();
        });
    </script>
</body>
</html>
            )";

            QByteArray responseBody;
            QString pageTitle, statusIndicator, heading, message;
            int closeDelay;

            if (!error.isEmpty() || receivedState != m_googleAuthState || authCode.isEmpty()) {
                pageTitle = "Sign-in failed";
                statusIndicator = "<div class=\"status-icon error-icon\">!</div>";
                heading = "Sign-in failed";
                closeDelay = 8;

                if (receivedState != m_googleAuthState) {
                    message = "<div class=\"error-message\">The sign-in request couldn't be verified. This may be due to a security issue.</div>"
                              "Please return to Cinephile and try signing in again.";
                } else if (!error.isEmpty()) {
                    message = QString("<div class=\"error-message\">%1</div>"
                                      "Please return to Cinephile and try signing in again.")
                                  .arg(error.toHtmlEscaped());
                } else {
                    message = "<div class=\"error-message\">The authorization process was incomplete.</div>"
                              "Please return to Cinephile and try signing in again.";
                }

                qWarning() << "Google OAuth Error/Mismatch. Error:" << error
                           << "State correct:" << (receivedState == m_googleAuthState);
            } else {
                pageTitle = "Sign-in successful";
                statusIndicator = "<div class=\"status-icon success-icon\">✓</div>";
                heading = "You're all set!";
                message = "<div class=\"success-message\">You've successfully signed in to Cinephile with your Google account.</div>"
                          "Your account is now connected and ready to use.";
                closeDelay = 5;
            }

            responseBody = htmlTemplate.arg(pageTitle)
                               .arg(statusIndicator)
                               .arg(heading)
                               .arg(message)
                               .arg(closeDelay)
                               .toUtf8();

            QByteArray httpResponse = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/html; charset=utf-8\r\n"
                                      "Content-Length: "
                                      + QByteArray::number(responseBody.length())
                                      + "\r\n"
                                        "Connection: close\r\n\r\n"
                                      + responseBody;
            m_tcpSocket->write(httpResponse);
            m_tcpSocket->flush();

            QTimer::singleShot(100, this, [this]() {
                if (m_tcpSocket)
                    m_tcpSocket->disconnectFromHost();
                if (m_tcpServer && m_tcpServer->isListening()) {
                    m_tcpServer->close();
                    qDebug() << "Local TCP server closed.";
                }
            });

            if (!error.isEmpty()) {
                qWarning() << "Google OAuth Error (from redirect):" << error;
                showLoginError("Google Sign-In failed: " + error);
                m_currentAuthOperation = AuthOperation::None;
                return;
            }
            if (receivedState != m_googleAuthState) {
                qWarning() << "Google OAuth State mismatch! Possible CSRF attack.";
                showLoginError("Google Sign-In security check failed (state mismatch).");
                m_currentAuthOperation = AuthOperation::None;
                return;
            }
            if (!authCode.isEmpty()) {
                qDebug() << "Google Auth Code received:" << authCode.left(15) << "...";
                exchangeGoogleCodeForToken(authCode);
            } else {
                qWarning() << "Google Auth Code not found in redirect.";
                showLoginError("Failed to get authorization code from Google.");
                m_currentAuthOperation = AuthOperation::None;
            }
        } else {
            if (m_tcpSocket)
                m_tcpSocket->disconnectFromHost();
        }
    } else {
        if (m_tcpSocket)
            m_tcpSocket->disconnectFromHost();
    }
}

void LoginWidget::handleTcpSocketDisconnected()
{
    if (m_tcpSocket) {
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
        qDebug() << "TCP socket for redirect disconnected and cleaned up.";
    }
}

void LoginWidget::exchangeGoogleCodeForToken(const QString &authCode)
{
    qDebug() << "Exchanging Google Auth Code for tokens...";
    m_currentAuthOperation = AuthOperation::GoogleTokenExchange;
    if (isLoginVisible && loginForm) {
        loginForm->errorLabel->setText("Verifying Google Sign-In...");
        loginForm->errorLabel->setVisible(true);
    } else if (registrationForm) {
        registrationForm->errorLabel->setText("Verifying Google Sign-In...");
        registrationForm->errorLabel->setVisible(true);
    }

    QUrlQuery postData;
    postData.addQueryItem("code", authCode);
    postData.addQueryItem("client_id", m_googleClientId);
    postData.addQueryItem("client_secret", m_googleClientSecret);
    postData.addQueryItem("code_verifier", m_googleCodeVerifier);
    postData.addQueryItem("grant_type", "authorization_code");
    postData.addQueryItem("redirect_uri", QString("http://localhost:%1").arg(m_redirectPort));
    QUrl tokenUrl("https://oauth2.googleapis.com/token");
    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray requestDataBytes = postData.toString(QUrl::FullyEncoded).toUtf8();
    qDebug() << "Token Request Data: " << QString(requestDataBytes).left(200) << "...";

    QNetworkReply *tokenReply = m_networkManager->post(request, requestDataBytes);
    connect(tokenReply, &QNetworkReply::finished, this, [this, tokenReply]() {
        onGoogleTokenReplyFinished(tokenReply);
    });
}

void LoginWidget::onGoogleTokenReplyFinished(QNetworkReply *reply)
{
    if (!reply) {
        showLoginError("Google token exchange: Network reply is null.");
        m_currentAuthOperation = AuthOperation::None;
        return;
    }
    QByteArray responseData = reply->readAll();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QNetworkReply::NetworkError networkError = reply->error();
    QString replyErrorString = reply->errorString();

    if (networkError != QNetworkReply::NoError
        && networkError != QNetworkReply::ContentOperationNotPermittedError) {

        qWarning() << "Google Token Exchange Network Error:" << replyErrorString
                   << "(Code:" << networkError << ")";

        showLoginError("Failed to get Google token: " + replyErrorString);
        m_currentAuthOperation = AuthOperation::None;
        reply->deleteLater();
        return;
    }

    if (responseData.isEmpty() && httpStatus != 200) {
        qWarning() << "Google Token Exchange: Empty response received with HTTP status"
                   << httpStatus;
        showLoginError("Empty response from Google token server (Status: "
                       + QString::number(httpStatus) + ").");
        m_currentAuthOperation = AuthOperation::None;
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Google Token Exchange: Invalid JSON response:" << responseData;
        showLoginError("Invalid response from Google token server.");
        m_currentAuthOperation = AuthOperation::None;
        reply->deleteLater();
        return;
    }

    QJsonObject json = jsonDoc.object();

    if (json.contains("error")) {
        QString error = json["error"].toString();
        QString errorDesc = json["error_description"].toString();
        qWarning() << "Google Token Exchange Error:" << error << "-" << errorDesc;
        showLoginError("Google Sign-In Error: " + errorDesc);
        m_currentAuthOperation = AuthOperation::None;
        reply->deleteLater();
        return;
    }

    QString googleIdToken = json["id_token"].toString();

    if (googleIdToken.isEmpty()) {
        qWarning() << "Google ID Token not found in token exchange response.";
        showLoginError("Failed to retrieve Google ID Token.");
        m_currentAuthOperation = AuthOperation::None;
        reply->deleteLater();
        return;
    }

    processFirebaseSignInWithGoogleIdToken(googleIdToken);
    reply->deleteLater();
}

void LoginWidget::processFirebaseSignInWithGoogleIdToken(const QString &googleIdToken)
{
    qDebug() << "Processing Firebase Sign-In with Google ID Token...";
    m_currentAuthOperation = AuthOperation::GoogleSignInIdP;
    m_pendingAuthDataContext.isGoogleSignIn = true;

    if (isLoginVisible && loginForm) {
        loginForm->errorLabel->setText("Verifying with Cinephile...");
        loginForm->errorLabel->setVisible(true);
    } else if (registrationForm) {
        registrationForm->errorLabel->setText("Verifying with Cinephile...");
        registrationForm->errorLabel->setVisible(true);
    }

    QJsonObject requestBody;
    QString postBodyStr = QString("id_token=%1&providerId=google.com").arg(googleIdToken);
    requestBody["postBody"] = postBodyStr;
    requestBody["returnSecureToken"] = true;
    requestBody["requestUri"]
        = QString("http://localhost:%1")
              .arg(m_redirectPort);

    QJsonDocument jsonDoc(requestBody);
    QByteArray jsonData = jsonDoc.toJson();

    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithIdp?key=%1")
                 .arg(m_firebaseApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_networkManager->post(request, jsonData);
}

QWidget *LoginWidget::createCustomWindowControls()
{
    QWidget *controlWidget = new QWidget(this);
    controlWidget->setObjectName("customControlWidget");
    QHBoxLayout *layout = new QHBoxLayout(controlWidget);
    layout->setContentsMargins(0, 0, 10, 5);
    layout->setSpacing(5);
    layout->addStretch();
    internalMinimizeButton = new QPushButton("—", controlWidget);
    internalMinimizeButton->setObjectName("internalMinimizeButton");
    internalMinimizeButton->setFixedSize(28, 28);
    internalMinimizeButton->setFlat(true);
    internalMinimizeButton->setToolTip("Minimize");
    connect(internalMinimizeButton,
            &QPushButton::clicked,
            this,
            &LoginWidget::onInternalMinimizeClicked);
    layout->addWidget(internalMinimizeButton);
    internalCloseButton = new QPushButton("✕", controlWidget);
    internalCloseButton->setObjectName("internalCloseButton");
    internalCloseButton->setFixedSize(28, 28);
    internalCloseButton->setFlat(true);
    internalCloseButton->setToolTip("Close");
    connect(internalCloseButton, &QPushButton::clicked, this, &LoginWidget::onInternalCloseClicked);
    layout->addWidget(internalCloseButton);
    controlWidget->setFixedHeight(35);
    return controlWidget;
}

void LoginWidget::setupUI()
{
    QStackedLayout *outerMostLayout = new QStackedLayout(this);
    outerMostLayout->setStackingMode(QStackedLayout::StackAll);

    panelsFrame = new QFrame(this);
    panelsFrame->setObjectName("panelsFrame");
    panelsFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    outerMostLayout->addWidget(panelsFrame);
    formPanelContainer = new QWidget(panelsFrame);
    formPanelContainer->setObjectName("formPanelContainer");

    QStackedLayout *formStack = new QStackedLayout();
    loginForm = new LoginForm();
    registrationForm = new RegistrationForm();

    formStack->addWidget(loginForm);
    formStack->addWidget(registrationForm);
    formPanelContainer->setLayout(formStack);

    overlayPanel = createOverlayPanel(false);
    overlayPanel->setParent(panelsFrame);

    QWidget *controls = createCustomWindowControls();
    outerMostLayout->addWidget(controls);
    controls->raise();
    setLayout(outerMostLayout);
}

QWidget *LoginWidget::createOverlayPanel(bool isLoginInvitePanel)
{
    QWidget *panel = new QWidget();
    panel->setObjectName("overlayPanel");

    QVBoxLayout *overlayLayout = new QVBoxLayout(panel);
    overlayLayout->setContentsMargins(50, 80, 50, 80);
    overlayLayout->setSpacing(25);
    overlayLayout->setAlignment(Qt::AlignCenter);

    overlayTitle = new QLabel(panel);
    overlayTitle->setObjectName("overlayTitle");
    overlayTitle->setAlignment(Qt::AlignCenter);
    overlayTitle->setWordWrap(true);

    overlayText = new QLabel(panel);
    overlayText->setObjectName("overlayText");
    overlayText->setAlignment(Qt::AlignCenter);
    overlayText->setWordWrap(true);

    overlayButton = new QPushButton(panel);
    overlayButton->setObjectName("overlayButton");
    overlayButton->setMinimumSize(160, 45);
    overlayButton->setCursor(Qt::PointingHandCursor);

    overlayLayout->addStretch(1);
    overlayLayout->addWidget(overlayTitle);
    overlayLayout->addWidget(overlayText);
    overlayLayout->addSpacing(15);
    overlayLayout->addWidget(overlayButton, 0, Qt::AlignCenter);
    overlayLayout->addStretch(1);

    updateOverlayContent(isLoginInvitePanel);
    return panel;
}

void LoginWidget::updateOverlayContent(bool showLoginInvite)
{
    overlayButton->disconnect();

    if (showLoginInvite) {
        overlayTitle->setText("One of Us?");
        overlayText->setText("If you already have an account, just sign in. We've missed you!");
        overlayButton->setText("SIGN IN");
        connect(overlayButton, &QPushButton::clicked, this, &LoginWidget::slideToLogin);
    } else {
        overlayTitle->setText("New Here?");
        overlayText->setText("Sign up and discover a great amount of new opportunities!");
        overlayButton->setText("SIGN UP");
        connect(overlayButton, &QPushButton::clicked, this, &LoginWidget::slideToRegister);
    }
}

void LoginWidget::applyStyles()
{
    QString styleSheet
        = QString(
              "QWidget#loginWidgetBase { background-color: transparent; }"
              "QFrame#panelsFrame { background-color: transparent; border: none; }"
              "QWidget#formPanelContainer { background-color: %1; border-radius: 10px; }"
              "QWidget#overlayPanel { background-color: %2; border-radius: 10px; }"
              "QWidget#formWidget { background-color: transparent; }"
              "QLabel#formTitle { color: %3; font-size: 28px; font-weight: bold; margin-bottom: "
              "20px; }"
              "QLabel#formSeparator, QLabel#orSeparator { color: %4; font-size: 12px; margin-top: "
              "15px; margin-bottom: 15px; }"
              "QPushButton#formActionButton {"
              "   background-color: %3;"
              "   color: %6;"
              "   border: none;"
              "   border-radius: 8px;"
              "   padding: 10px 25px;"
              "   font-size: 14px;"
              "   font-weight: bold;"
              "   margin-top: 15px;"
              "   min-width: 150px;"
              "}"
              "QPushButton#formActionButton:hover { background-color: %7; }"
              "QPushButton#formActionButton:pressed { background-color: %5; }"
              "QPushButton#formActionButton:disabled { background-color: #CCCCCC; color: #666666; }"
              "QPushButton#googleButton_log, QPushButton#googleButton_reg {"
              "   background-color: %6;"
              "   color: #333333;"
              "   border: 1px solid #CCCCCC;"
              "   border-radius: 20px;"
              "   padding: 10px 20px 10px 15px;"
              "   font-size: 14px;"
              "   font-weight: 500;"
              "   margin-top: 5px;"
              "   min-width: 170px;"
              "   qproperty-iconSize: 20px 20px;"
              "   text-align: center;"
              "}"
              "QPushButton#googleButton_log:hover, QPushButton#googleButton_reg:hover { "
              "background-color: #f9f9f9; border-color: #bbbbbb; }"
              "QPushButton#googleButton_log:pressed, QPushButton#googleButton_reg:pressed { "
              "background-color: #f1f1f1; }"
              "QLabel#errorLabel { color: %3; font-size: 12px; font-weight: 500; min-height: 16px; "
              "}"
              "QWidget#inputField { background-color: %12; border-radius: 6px; border: 1px solid "
              "%11; }"
              "QWidget#inputField:focus-within { border: 1px solid %13; }"
              "QLabel#inputIcon { background-color: transparent; }"
              "QLineEdit#inputText { background-color: transparent; border: none; color: %4; "
              "font-size: 14px; padding: 8px 5px; selection-background-color: %3; selection-color: "
              "%6; }"
              "QLineEdit#inputText::placeholder { color: %14; }"
              "QLabel#overlayTitle { color: #EEEBDD; font-size: 28px; font-weight: bold; }"
              "QLabel#overlayText { color: #EEEBDD; font-size: 14px; }"
              "QPushButton#overlayButton {"
              "   background-color: transparent;"
              "   color: #EEEBDD;"
              "   border: 2px solid #EEEBDD;"
              "   border-radius: 8px;"
              "   padding: 10px 25px;"
              "   font-size: 14px;"
              "   font-weight: bold;"
              "}"
              "QPushButton#overlayButton:hover {"
              "   background-color: %6;"
              "   border-color: %6;"
              "   color: %2;"
              "}"
              "QPushButton#overlayButton:pressed {"
              "   background-color: rgba(255, 255, 255, 0.9);"
              "   color: %2;"
              "   border-color: rgba(255, 255, 255, 0.9);"
              "}"
              "QWidget#customControlWidget { background-color: transparent; }"
              "QPushButton#internalMinimizeButton, QPushButton#internalCloseButton { "
              "background-color: transparent; border: none; border-radius: 4px; padding: 4px; "
              "color: #EEEBDD; font-size: 14px; font-weight: bold; }"
              "QPushButton#internalMinimizeButton:hover { background-color: #666666; }"
              "QPushButton#internalCloseButton:hover { background-color: #CE1212; }"
              "QPushButton#internalMinimizeButton:pressed { background-color: %20; }"
              "QPushButton#internalCloseButton:pressed { background-color: %21; }")
              .arg(AppTheme::PANEL_LIGHT,
                   AppTheme::BACKGROUND_DARK,
                   AppTheme::PRIMARY_RED,
                   AppTheme::TEXT_ON_LIGHT,
                   AppTheme::DARK_RED,
                   AppTheme::BUTTON_WHITE_BG,
                   AppTheme::BUTTON_HOVER_LIGHT_BG,
                   AppTheme::BUTTON_HOVER_DARK_BG,
                   AppTheme::BUTTON_DISABLED_BG,
                   AppTheme::TEXT_DISABLED,
                   AppTheme::BUTTON_BORDER_LIGHT,
                   AppTheme::INPUT_BG,
                   AppTheme::INPUT_BORDER_FOCUS,
                   AppTheme::TEXT_PLACEHOLDER,
                   AppTheme::TEXT_ON_DARK,
                   AppTheme::OVERLAY_BUTTON_HOVER_BG,
                   AppTheme::OVERLAY_BUTTON_PRESSED_BG,
                   AppTheme::BUTTON_HOVER_GENERAL_BG,
                   AppTheme::BUTTON_CLOSE_HOVER_BG,
                   AppTheme::BUTTON_PRESSED_GENERAL_BG,
                   AppTheme::BUTTON_CLOSE_PRESSED_BG);
    this->setStyleSheet(styleSheet);

    if (panelsFrame) {
        if (panelsFrame->graphicsEffect()) {
            delete panelsFrame->graphicsEffect();
        }

        auto shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(25);
        shadow->setXOffset(0);
        shadow->setYOffset(4);
        shadow->setColor(AppTheme::SHADOW_COLOR);

        panelsFrame->setGraphicsEffect(shadow);
    }
}
void LoginWidget::setupAnimations()
{
    int duration = 450;
    QEasingCurve curve = QEasingCurve::InOutCubic;

    if (!overlayPanel)
        return;
    if (!overlayAnimation) {
        overlayAnimation = new QPropertyAnimation(overlayPanel, "pos", this);
    }

    overlayAnimation->setDuration(duration);
    overlayAnimation->setEasingCurve(curve);
}

void LoginWidget::setupInitialLayout()
{
    if (m_initialPositionsSet)
        return;
    if (!panelsFrame || !overlayPanel || !formPanelContainer || !loginForm || !registrationForm
        || !this->layout()) {
        qWarning() << "setupInitialLayout: Required widgets or layout missing.";
        QTimer::singleShot(10, this, &LoginWidget::setupInitialLayout);
        return;
    }
    const int initialWidth = 850;
    const int initialHeight = 600;
    if (qAbs(this->width() - initialWidth) > 50 || qAbs(this->height() - initialHeight) > 50) {
        qWarning() << "setupInitialLayout: Widget size" << this->size()
                   << "differs significantly from target" << initialWidth << "x" << initialHeight
                   << ". Retrying...";
        QTimer::singleShot(20, this, &LoginWidget::setupInitialLayout);
        return;
    }
    int panelWidth = initialWidth / 2;
    int panelHeight = initialHeight;
    qDebug() << "setupInitialLayout: Setting initial panel size based on target:" << panelWidth
             << "x" << panelHeight;
    formPanelContainer->setFixedSize(panelWidth, panelHeight);
    overlayPanel->setFixedSize(panelWidth, panelHeight);
    QStackedLayout *formStack = static_cast<QStackedLayout *>(formPanelContainer->layout());
    if (!formStack)
        return;
    if (isLoginVisible) {
        formPanelContainer->move(0, 0);
        overlayPanel->move(panelWidth, 0);
        formStack->setCurrentWidget(loginForm);
        updateOverlayContent(false);
    } else {
        formPanelContainer->move(panelWidth, 0);
        overlayPanel->move(0, 0);
        formStack->setCurrentWidget(registrationForm);
        updateOverlayContent(true);
    }
    m_initialPositionsSet = true;
    QWidget *controls = findChild<QWidget *>("customControlWidget");
    if (controls) {
        int controlsWidth = controls->sizeHint().width();
        int controlsHeight = controls->sizeHint().height();
        if (controlsWidth <= 0)
            controlsWidth = 80;
        if (controlsHeight <= 0)
            controlsHeight = 35;
        controls->setGeometry(initialWidth - controlsWidth - 10, 5, controlsWidth, controlsHeight);
        controls->raise();
        qDebug() << "Controls positioned based on initial width.";
    }
}

void LoginWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!m_initialPositionsSet) {
        QTimer::singleShot(0, this, &LoginWidget::setupInitialLayout);
    }
}

void LoginWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (m_initialPositionsSet) {
        int newWidth = event->size().width();
        int newHeight = event->size().height();
        int panelWidth = newWidth / 2;
        int panelHeight = newHeight;
        if (formPanelContainer)
            formPanelContainer->setFixedSize(panelWidth, panelHeight);
        if (overlayPanel)
            overlayPanel->setFixedSize(panelWidth, panelHeight);
        if (isLoginVisible) {
            if (formPanelContainer)
                formPanelContainer->move(0, 0);
            if (overlayPanel)
                overlayPanel->move(panelWidth, 0);
        } else {
            if (overlayPanel)
                overlayPanel->move(0, 0);
            if (formPanelContainer)
                formPanelContainer->move(panelWidth, 0);
        }

        QWidget *controls = findChild<QWidget *>("customControlWidget");

        if (controls)
            controls->move(newWidth - controls->width() - 10, controls->y());
    } else {
        QTimer::singleShot(0, this, &LoginWidget::setupInitialLayout);
    }
}

void LoginWidget::slideToRegister()
{
    if (!m_initialPositionsSet || !overlayAnimation || !isLoginVisible || !panelsFrame
        || !formPanelContainer || !registrationForm)
        return;
    int panelWidth = panelsFrame->width() / 2;
    if (panelWidth <= 0)
        return;
    isLoginVisible = false;
    formPanelContainer->move(panelWidth, 0);
    overlayAnimation->stop();
    overlayAnimation->setStartValue(overlayPanel->pos());
    overlayAnimation->setEndValue(QPoint(0, 0));
    overlayAnimation->start();
    updateOverlayContent(true);
    int delayMs = 150;
    QTimer::singleShot(delayMs, this, [this]() {
        if (registrationForm) {
            static_cast<QStackedLayout *>(formPanelContainer->layout())
                ->setCurrentWidget(registrationForm);
            registrationForm->clearForm();
        }
    });
}

void LoginWidget::slideToLogin()
{
    if (!m_initialPositionsSet || !overlayAnimation || isLoginVisible || !panelsFrame
        || !formPanelContainer || !loginForm)
        return;
    int panelWidth = panelsFrame->width() / 2;
    if (panelWidth <= 0)
        return;
    isLoginVisible = true;
    formPanelContainer->move(0, 0);
    overlayAnimation->stop();
    overlayAnimation->setStartValue(overlayPanel->pos());
    overlayAnimation->setEndValue(QPoint(panelWidth, 0));
    overlayAnimation->start();
    updateOverlayContent(false);
    int delayMs = 150;
    QTimer::singleShot(delayMs, this, [this]() {
        if (loginForm) {
            static_cast<QStackedLayout *>(formPanelContainer->layout())->setCurrentWidget(loginForm);
            loginForm->clearForm();
        }
    });
}

void LoginWidget::onLoginValidate()
{
    if (!loginForm || !loginForm->loginButton)
        return;
    bool emailOk = !loginForm->emailInput->text().trimmed().isEmpty();
    bool passOk = !loginForm->passwordInput->text().isEmpty();
    loginForm->loginButton->setEnabled(emailOk && passOk);
    if (loginForm->errorLabel->isVisible() && (emailOk || passOk)) {
        loginForm->errorLabel->setVisible(false);
    }
}

void LoginWidget::onRegisterValidate()
{
    if (!registrationForm || !registrationForm->registerButton)
        return;
    bool nameOk = !registrationForm->nameInput->text().trimmed().isEmpty();
    bool emailOk = !registrationForm->emailInput->text().trimmed().isEmpty();
    bool passOk = !registrationForm->passwordInput->text().isEmpty();
    registrationForm->registerButton->setEnabled(nameOk && emailOk && passOk);
    if (registrationForm->errorLabel->isVisible() && (nameOk || emailOk || passOk)) {
        registrationForm->errorLabel->setVisible(false);
    }
}

void LoginWidget::showLoginError(const QString &message)
{
    if (loginForm && loginForm->errorLabel) {
        loginForm->errorLabel->setText(message);
        loginForm->errorLabel->setVisible(true);
    }
}

void LoginWidget::showRegistrationError(const QString &message)
{
    if (registrationForm && registrationForm->errorLabel) {
        registrationForm->errorLabel->setText(message);
        registrationForm->errorLabel->setVisible(true);
    }
}

void LoginWidget::clearLoginPassword()
{
    if (loginForm && loginForm->passwordInput) {
        loginForm->passwordInput->clear();
    }
}

void LoginWidget::clearRegistrationPasswords()
{
    if (registrationForm && registrationForm->passwordInput) {
        registrationForm->passwordInput->clear();
    }
}

void LoginWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget *child = childAt(event->position().toPoint());
    bool isInteractive = false;
    while (child != nullptr && child != this) {
        if (qobject_cast<QPushButton *>(child) || qobject_cast<QLineEdit *>(child)) {
            isInteractive = true;
            break;
        }
        child = qobject_cast<QWidget *>(child->parent());
    }
    QWidget *controls = findChild<QWidget *>("customControlWidget");
    if (controls && controls->geometry().contains(event->position().toPoint())) {
        isInteractive = true;
    }
    if (!isInteractive && event->button() == Qt::LeftButton && window()) {
        m_dragPosition = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
        return;
    }
    m_dragging = false;
    QWidget::mousePressEvent(event);
}

void LoginWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton) && window()) {
        window()->move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void LoginWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void LoginWidget::onInternalMinimizeClicked()
{
    emit minimizeApp();
}

void LoginWidget::onInternalCloseClicked()
{
    emit closeApp();
}
