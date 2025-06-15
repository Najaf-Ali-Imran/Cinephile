// --- START OF FILE firestoreservice.h ---

#ifndef FIRESTORESERVICE_H
#define FIRESTORESERVICE_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVariantMap>

class QNetworkReply;

QJsonObject valueToJson(const QVariant &value);
QVariant jsonToValue(const QJsonObject &valueObject);
QJsonObject fieldsToJson(const QVariantMap &fieldsMap);
QJsonObject jsonToPlainObject(const QJsonObject &firestoreDoc);

class FirestoreService : public QObject
{
    Q_OBJECT
public:
    explicit FirestoreService(const QString &firebaseApiKey, QObject *parent = nullptr);

    void checkAndCreateUserProfile(const QString &uid,
                                   const QString &idToken,
                                   const QString &email,
                                   const QString &displayName);
    void fetchUserProfile(const QString &uid, const QString &idToken);
    void updateUserDocument(const QString &uid,
                            const QString &idToken,
                            const QVariantMap &fieldsToUpdate);

signals:
    void userProfileCheckCreateComplete(bool success,
                                        const QString &uid,
                                        const QJsonObject &profileData,
                                        const QString &error);
    void userProfileFetched(bool success,
                            const QString &uid,
                            const QJsonObject &profileData,
                            const QString &error);
    void userDocumentUpdated(bool success,
                             const QString &uid,
                             const QJsonObject &updatedFields,
                             const QString &error);

private slots:
    void onNetworkReply(QNetworkReply *reply);

private:
    QString m_projectId;
    QString m_apiKey;
    QNetworkAccessManager *m_networkManager;
    QString m_baseFirestoreUrl;

    enum class FirestoreOperation {
        Unknown,
        CheckUserProfile,
        CreateUserProfile,
        FetchUserProfile,
        UpdateUserDocument
    };

    struct PendingOperation
    {
        FirestoreOperation type;
        QString uid;
        QString idToken;
        QString email;
        QString displayName;
        QJsonObject originalPayload;
        QVariantMap fieldsToUpdate;
    };
    QMap<QNetworkReply *, PendingOperation> m_pendingOperations;

    QString buildFirestoreUrl(const QString &path = "") const;
    QString buildFirestorePatchUrl(const QString &documentPath,
                                   const QVariantMap &fieldsToUpdate) const;

    void sendFirestoreRequest(const QString &fullPath,
                              const QString &method,
                              const QString &idToken,
                              const QByteArray &payload,
                              const PendingOperation &opDetails);
};

#endif
