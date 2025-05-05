// --- START OF FILE LoginWidget.h ---

#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QtWidgets>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QMouseEvent>
#include <QPoint>
#include <QWidget>
#include <QResizeEvent>
#include <QTimer>


class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;
class LoginForm;
class RegistrationForm;
class InputField;


class InputField : public QWidget {
    Q_OBJECT
public:
    explicit InputField(const QString &iconPath, const QString &placeholder, bool isPassword = false, QWidget *parent = nullptr);
    QString text() const;
    void setText(const QString &text);
    void clear();
    void setFocus();
    QLineEdit* lineEdit() const { return m_lineEdit; }

signals:
    void textChanged(const QString &text);

private:
    QLabel *m_iconLabel;
    QLineEdit *m_lineEdit;
};


class RegistrationForm : public QWidget {
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
    QPushButton* createGoogleButton(const QString& objectName);
};


class LoginForm : public QWidget {
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
    QPushButton* createGoogleButton(const QString& objectName);
};


class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);

signals:

    void loginRequested(const QString &email, const QString &password);
    void registrationRequested(const QString &name, const QString &email, const QString &password);
    void googleSignInRequested();
    void minimizeApp();
    void closeApp();

public slots:

    void slideToRegister();
    void slideToLogin();
    void showLoginError(const QString& message);
    void showRegistrationError(const QString& message);
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
    void relayGoogleSignIn();
    void onInternalMinimizeClicked();
    void onInternalCloseClicked();

private:

    QWidget* createOverlayPanel(bool isLoginPanel);
    QWidget* createCustomWindowControls();
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
};

#endif // LOGINWIDGET_H
// --- END OF FILE LoginWidget.h ---
