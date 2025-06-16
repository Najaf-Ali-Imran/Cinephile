#ifndef PROFILEPAGEWIDGET_H
#define PROFILEPAGEWIDGET_H

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QScrollArea;
class ClickableLabel;
class QFrame;
class LoginWidget;
class FirestoreService;

class ProfilePageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProfilePageWidget(LoginWidget *authService,
                               FirestoreService *firestoreService,
                               QWidget *parent = nullptr);
    ~ProfilePageWidget();

    void loadProfileData();

signals:
    void logoutRequested();

private slots:
    void onProfilePictureClicked();
    void onSaveDisplayNameClicked();
    void onSaveEmailClicked();
    void onChangePasswordClicked();
    void onLogoutClicked();
    void updateUIFromUserManager();

    void handleProfileAuthUpdateSuccess(const QString &operation, const QJsonObject &responseData);
    void handleProfileAuthUpdateFailed(const QString &operation, const QString &error);
    void handleFirestoreUpdateSuccess(bool success,
                                      const QString &uid,
                                      const QJsonObject &updatedFields,
                                      const QString &error);

private:
    void setupUi();
    QWidget *createSectionHeader(const QString &title);
    QWidget *createEditableField(const QString &labelText,
                                 QLineEdit **field,
                                 QPushButton **button,
                                 const QString &buttonText,
                                 const QString &placeholder = "");
    QFrame *createDivider();
    void loadProfilePictureFromLocalPath(const QString &localPath);
    void setProfilePicture(const QPixmap &pixmap);
    QString getProfilePicStoragePath() const;
    QString formatDisplayNameForStorage(const QString &name);

    LoginWidget *m_authService;
    FirestoreService *m_firestoreService;

    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;

    ClickableLabel *m_profilePictureLabel;
    QLabel *m_displayNameHeaderLabel;
    QLabel *m_emailHeaderLabel;

    QLineEdit *m_editDisplayNameLineEdit;
    QPushButton *m_saveDisplayNameButton;

    QLineEdit *m_editEmailLineEdit;
    QLineEdit *m_currentPasswordForEmailLineEdit;
    QPushButton *m_saveEmailButton;

    QLineEdit *m_currentPasswordLineEdit;
    QLineEdit *m_newPasswordLineEdit;
    QLineEdit *m_confirmNewPasswordLineEdit;
    QPushButton *m_changePasswordButton;

    QLabel *m_statusMessageLabel;

    QPushButton *m_logoutButton;

    QString m_initialDisplayName;
    QString m_initialEmail;
    QString m_currentLocalPicPath;
};

#endif
