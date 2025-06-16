#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

class FirestoreService;

class UserManager : public QObject
{
    Q_OBJECT
private:
    explicit UserManager(QObject *parent = nullptr);
    ~UserManager();

    UserManager(const UserManager &) = delete;
    UserManager &operator=(const UserManager &) = delete;

    QString m_uid;
    QString m_idToken;
    QString m_refreshToken;
    QString m_email;
    QString m_displayName;
    QString m_profilePictureLocalPath;
    QJsonObject m_profileData;
    bool m_isAuthenticated;
    bool m_isGoogleSignIn;
    QString m_firebaseApiKey;

    FirestoreService* m_firestoreService;
    void initializeFirestoreService();

    static UserManager *s_instance;

public:
    static UserManager *instance();

    void setApiKey(const QString &apiKey);
    FirestoreService* firestoreService();

    void setCurrentUser(const QString &uid,
                        const QString &idToken,
                        const QString &refreshToken,
                        const QString &email,
                        const QString &displayName,
                        bool isGoogleSignIn = false);
    void clearCurrentUser();

    void setProfileData(const QJsonObject &profileData);
    QJsonObject getProfileData() const;

    QString getUid() const;
    QString getIdToken() const;
    void updateIdToken(const QString &newIdToken);
    QString getRefreshToken() const;
    QString getEmail() const;
    void setEmail(const QString &email);

    QString getDisplayName() const;
    void setDisplayName(const QString &displayName);

    QString getProfilePictureUrl() const;
    void setProfilePictureUrl(const QString &localPath);

    bool isAuthenticated() const;
    bool isGoogleSignIn() const;

    void addMovieToList(int movieId, const QString &listName);
    void removeMovieFromList(int movieId, const QString &listName);
    void createCustomList(const QString &listName);
    void deleteCustomList(const QString &listName, const QVariantList& movieIds);
    void renameCustomList(const QString &oldName, const QString &newName);
    void updateMovieInLists(int movieId, const QStringList &finalLists);

    QStringList getAllListNames(bool includePredefined = true, bool includeCustom = true) const;
    QStringList getListsForMovie(int movieId) const;
    bool isMovieInList(int movieId, const QString &listName) const;


signals:
    void userAuthenticatedChanged(bool isAuthenticated);
    void displayNameChanged(const QString &newDisplayName);
    void emailChanged(const QString &newEmail);
    void profilePictureChanged(const QString &newProfilePictureLocalPath);
    void idTokenRefreshed(const QString &newIdToken);
    void profileDataChanged();
};

#endif
