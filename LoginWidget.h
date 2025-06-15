#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QMouseEvent>
#include <QPoint>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>
#include <QtWidgets>
#include "ConfigManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QFileDialog>
#include <QTcpServer>
#include <QTcpSocket>

class FirestoreService;

class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;
class LoginForm;
class RegistrationForm;
class InputField;

class InputField : public QWidget
{
    Q_OBJECT
public:
    explicit InputField(const QString &iconPath,
                        const QString &placeholder,
                        bool isPassword = false,
                        QWidget *parent = nullptr);
    QString text() const;
    void setText(const QString &text);
    void clear();
    void setFocus();
    QLineEdit *lineEdit() const {
        return m_lineEdit;
    }

signals:
    void textChanged(const QString &text);

private:
    QLabel *m_iconLabel;
    QLineEdit *m_lineEdit;
};

class RegistrationForm : public QWidget
{
    Q_OBJECT
public:
    explicit RegistrationForm(QWidget *parent = nullptr);
    InputField *nameInput;
    InputField *emailInput;
    InputField *passwordInput;
    QPushButton *registerButton;
    QPushButton *googleButton_reg;
    QLabel *errorLabel;
    void clearForm();
    QString getFullName() const;

signals:
    void registrationAttempted(const QString &name, const QString &email, const QString &password);
    void validateFormSignal();
    void googleSignInRequested_reg();

private slots:
    void onRegisterClicked();
    void onInputChanged();
    void onGoogleClicked_reg();

private:
    void setupUI();
    QPushButton *createGoogleButton(const QString &objectName);
};

class LoginForm : public QWidget
{
    Q_OBJECT
public:
    explicit LoginForm(QWidget *parent = nullptr);
    InputField *emailInput;
    InputField *passwordInput;
    QPushButton *loginButton;
    QPushButton *googleButton_log;
    QLabel *errorLabel;
    void clearForm();

signals:
    void loginAttempted(const QString &email, const QString &password);
    void googleSignInRequested_log();
    void validateFormSignal();

private slots:
    void onLoginClicked();
    void onGoogleClicked_log();
    void onInputChanged();

private:
    void setupUI();
    QPushButton *createGoogleButton(const QString &objectName);
};

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

    void updateUserProfile(const QString &idToken,
                           const QString &displayName,
                           const QString &photoUrl = "");
    void updateUserEmail(
        const QString &idToken,
        const QString &newEmail,
        const QString
            &currentPassword);
    void updateUserPassword(const QString &idToken, const QString &newPassword);

signals:
    void userAuthenticated(const QString &uid,
                           const QString &idToken,
                           const QString &refreshToken,
                           const QString &email,
                           const QString &displayName);
    void minimizeApp();
    void closeApp();

    void profileUpdateSuccess(const QString &operation, const QJsonObject &responseData);
    void profileUpdateFailed(const QString &operation, const QString &error);

public slots:
    void slideToRegister();
    void slideToLogin();
    void showLoginError(const QString &message);
    void showRegistrationError(const QString &message);
    void clearLoginPassword();
    void clearRegistrationPasswords();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void setupInitialLayout();
    void onLoginValidate();
    void onRegisterValidate();
    void handleGoogleSignInRequested();
    void onInternalMinimizeClicked();
    void onInternalCloseClicked();

    void handleRegistrationAttemptFromForm(const QString &name,
                                           const QString &email,
                                           const QString &password);
    void handleLoginAttemptFromForm(const QString &email, const QString &password);
    void onAuthReplyFinished(QNetworkReply *reply);

    void handleNewTcpConnection();
    void handleTcpSocketReadyRead();
    void handleTcpSocketDisconnected();
    void onGoogleTokenReplyFinished(QNetworkReply *reply);

    void onFirestoreUserProfileChecked(bool success,
                                       const QString &uid,
                                       const QJsonObject &profileData,
                                       const QString &error);

private:
    QWidget *createOverlayPanel(bool isLoginPanel);
    QWidget *createCustomWindowControls();
    void setupUI();
    void applyStyles();
    void setupAnimations();
    void updateOverlayContent(bool showLoginInvite);

    QFrame *panelsFrame;
    QHBoxLayout *panelsLayout;
    QWidget *formPanelContainer;
    QWidget *overlayPanel;
    LoginForm *loginForm;
    RegistrationForm *registrationForm;
    QLabel *overlayTitle;
    QLabel *overlayText;
    QPushButton *overlayButton;
    QPushButton *internalMinimizeButton;
    QPushButton *internalCloseButton;
    QPropertyAnimation *overlayAnimation = nullptr;
    bool isLoginVisible;
    bool m_initialPositionsSet;
    QPoint m_dragPosition;
    bool m_dragging = false;

    QNetworkAccessManager *m_networkManager;
    QString m_firebaseApiKey;

    enum class AuthOperation {
        None,
        SignUp,
        SignIn,
        GoogleSignInIdP,
        GoogleTokenExchange,
        FirestoreProfileCheck,
        UpdateUserProfile,
        UpdateEmail,
        UpdatePassword
    };
    AuthOperation m_currentAuthOperation = AuthOperation::None;
    QString m_currentProfileUpdateOperationName;

    struct AuthDataContext
    {
        QString uid;
        QString idToken;
        QString refreshToken;
        QString email;
        QString displayNameFromAuth;
        bool isGoogleSignIn;
    };
    AuthDataContext m_pendingAuthDataContext;

    QTcpServer *m_tcpServer = nullptr;
    QTcpSocket *m_tcpSocket = nullptr;
    quint16 m_redirectPort = 0;
    QString m_googleAuthState;
    QString m_googleCodeVerifier;
    QString m_googleClientId = ConfigManager::instance()->getGoogleClientId();
    QString m_googleClientSecret = ConfigManager::instance()->getGoogleClientSecret();

    FirestoreService *m_firestoreService;

    void startGoogleOAuthFlow();
    void exchangeGoogleCodeForToken(const QString &authCode);
    void processFirebaseSignInWithGoogleIdToken(const QString &googleIdToken);

    // Updated declaration
    void proceedToApp(const QString &uid,
                      const QString &idToken,
                      const QString &refreshToken,
                      const QString &email,
                      const QString &displayName,
                      bool isGoogleSignIn);
};

#endif
