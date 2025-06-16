#include "UserManager.h"
#include "firestoreservice.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QVariantMap>

UserManager *UserManager::s_instance = nullptr;

UserManager::UserManager(QObject *parent)
    : QObject(parent)
    , m_profileData(QJsonObject())
    , m_isAuthenticated(false)
    , m_isGoogleSignIn(false)
    , m_firestoreService(nullptr)
{
}

UserManager::~UserManager()
{
    delete m_firestoreService;
}

UserManager *UserManager::instance()
{
    if (!s_instance) {
        s_instance = new UserManager();
    }
    return s_instance;
}

void UserManager::setApiKey(const QString &apiKey)
{
    if (m_firebaseApiKey != apiKey) {
        m_firebaseApiKey = apiKey;
        initializeFirestoreService();
    }
}

void UserManager::initializeFirestoreService()
{
    if (!m_firebaseApiKey.isEmpty()) {
        delete m_firestoreService;
        m_firestoreService = new FirestoreService(m_firebaseApiKey, this);
    }
}

FirestoreService *UserManager::firestoreService()
{
    return m_firestoreService;
}

void UserManager::setCurrentUser(const QString &uid,
                                 const QString &idToken,
                                 const QString &refreshToken,
                                 const QString &email,
                                 const QString &displayName,
                                 bool isGoogleSignIn)
{
    bool wasAuthenticated = m_isAuthenticated;
    QString oldDisplayName = m_displayName;
    QString oldEmail = m_email;
    QString oldPicPath = m_profilePictureLocalPath;

    m_uid = uid;
    m_idToken = idToken;
    m_refreshToken = refreshToken;
    m_email = email;
    m_displayName = displayName;
    m_isGoogleSignIn = isGoogleSignIn;
    m_isAuthenticated = true;

    if (m_profilePictureLocalPath.isEmpty()
        || !QFile::exists(m_profilePictureLocalPath)) {
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (dataPath.isEmpty())
            dataPath = QDir::homePath() + "/.CinephileAppData";
        QString profilePicDir = dataPath + "/profile_pictures";

        QString pngPath = profilePicDir + "/" + uid + ".png";
        QString jpgPath = profilePicDir + "/" + uid + ".jpg";
        QString jpegPath = profilePicDir + "/" + uid + ".jpeg";

        if (QFile::exists(pngPath))
            m_profilePictureLocalPath = pngPath;
        else if (QFile::exists(jpgPath))
            m_profilePictureLocalPath = jpgPath;
        else if (QFile::exists(jpegPath))
            m_profilePictureLocalPath = jpegPath;
        else
            m_profilePictureLocalPath.clear();
    }

    if (!wasAuthenticated || oldEmail != m_email) {
        emit emailChanged(m_email);
    }
    if (!wasAuthenticated || oldDisplayName != m_displayName) {
        emit displayNameChanged(m_displayName);
    }
    if (!wasAuthenticated || oldPicPath != m_profilePictureLocalPath) {
        emit profilePictureChanged(m_profilePictureLocalPath);
    }
    if (!wasAuthenticated) {
        emit userAuthenticatedChanged(true);
    }
}

void UserManager::clearCurrentUser()
{
    bool needsAuthChangeSignal = m_isAuthenticated;
    bool needsDisplayNameSignal = !m_displayName.isEmpty();
    bool needsEmailSignal = !m_email.isEmpty();
    bool needsPicSignal = !m_profilePictureLocalPath.isEmpty();

    m_uid.clear();
    m_idToken.clear();
    m_refreshToken.clear();
    m_email.clear();
    m_displayName.clear();
    m_profilePictureLocalPath.clear();
    m_profileData = QJsonObject();
    m_isAuthenticated = false;
    m_isGoogleSignIn = false;

    if (needsAuthChangeSignal)
        emit userAuthenticatedChanged(false);
    if (needsDisplayNameSignal)
        emit displayNameChanged("");
    if (needsEmailSignal)
        emit emailChanged("");
    if (needsPicSignal)
        emit profilePictureChanged("");
}

void UserManager::setProfileData(const QJsonObject &profileData)
{
    if (m_profileData != profileData) {
        m_profileData = profileData;
        emit profileDataChanged();
    }
}

QJsonObject UserManager::getProfileData() const
{
    return m_profileData;
}

void UserManager::addMovieToList(int movieId, const QString &listName)
{
    if (listName.isEmpty() || !m_isAuthenticated || !m_firestoreService) return;

    QJsonObject profile = m_profileData;
    QVariantMap updatePayload;

    if (listName == "watchlist" || listName == "favorites" || listName == "watchedHistory") {
        QVariantList list = profile[listName].toVariant().toList();
        if (!list.contains(movieId)) {
            list.append(movieId);
            updatePayload[listName] = list;
        }
    } else {
        QVariantMap customLists = profile["customLists"].toVariant().toMap();
        if (customLists.contains(listName)) {
            QVariantList list = customLists[listName].toList();
            if (!list.contains(movieId)) {
                list.append(movieId);
                customLists[listName] = list;
                updatePayload["customLists"] = customLists;
            }
        }
    }

    if (!updatePayload.isEmpty()) {
        m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
    }
}

void UserManager::removeMovieFromList(int movieId, const QString &listName)
{
    if (listName.isEmpty() || !m_isAuthenticated || !m_firestoreService) return;

    QJsonObject profile = m_profileData;
    QVariantMap updatePayload;
    bool listModified = false;

    if (listName == "watchlist" || listName == "favorites" || listName == "watchedHistory") {
        QVariantList list = profile[listName].toVariant().toList();
        if (list.removeAll(movieId) > 0) {
            updatePayload[listName] = list;
            listModified = true;
        }
    } else {
        QVariantMap customLists = profile["customLists"].toVariant().toMap();
        if (customLists.contains(listName)) {
            QVariantList list = customLists[listName].toList();
            if (list.removeAll(movieId) > 0) {
                customLists[listName] = list;
                updatePayload["customLists"] = customLists;
                listModified = true;
            }
        }
    }

    if (listModified) {
        m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
    }
}

void UserManager::createCustomList(const QString &listName)
{
    if (listName.isEmpty() || !m_isAuthenticated || !m_firestoreService) return;

    QJsonObject profile = m_profileData;
    QVariantMap customLists = profile["customLists"].toVariant().toMap();

    if (customLists.contains(listName)) {
        qWarning() << "UserManager: Custom list" << listName << "already exists.";
        return;
    }

    customLists[listName] = QVariantList();
    QVariantMap updatePayload;
    updatePayload["customLists"] = customLists;
    m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
}

void UserManager::deleteCustomList(const QString &listName, const QVariantList& movieIds)
{
    if (listName.isEmpty() || !m_isAuthenticated || !m_firestoreService) return;

    QJsonObject profile = m_profileData;
    QVariantMap customLists = profile["customLists"].toVariant().toMap();

    if (!customLists.contains(listName)) return;

    customLists.remove(listName);
    QVariantMap updatePayload;
    updatePayload["customLists"] = customLists;
    m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
}


void UserManager::renameCustomList(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty() || oldName == newName || !m_isAuthenticated || !m_firestoreService) return;

    QJsonObject profile = m_profileData;
    QVariantMap customLists = profile["customLists"].toVariant().toMap();

    if (!customLists.contains(oldName) || customLists.contains(newName)) {
        qWarning() << "UserManager: Cannot rename" << oldName << "to" << newName << "- old doesn't exist or new already exists.";
        return;
    }

    QVariant listContent = customLists.take(oldName);
    customLists[newName] = listContent;

    QVariantMap updatePayload;
    updatePayload["customLists"] = customLists;
    m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
}

void UserManager::updateMovieInLists(int movieId, const QStringList &finalLists)
{
    if (!m_isAuthenticated || !m_firestoreService)
        return;

    QJsonObject profile = m_profileData;
    QVariantMap updatePayload;
    bool changed = false;

    QVariantList watchlist = profile["watchlist"].toVariant().toList();
    QVariantList favorites = profile["favorites"].toVariant().toList();
    QVariantList history = profile["watchedHistory"].toVariant().toList();
    QVariantMap customLists = profile["customLists"].toVariant().toMap();

    auto updateList = [&](QVariantList& list, const QString& listName) {
        bool isInFinal = finalLists.contains(listName);
        bool isInCurrent = list.contains(movieId);
        if (isInFinal && !isInCurrent) {
            list.append(movieId);
            changed = true;
        } else if (!isInFinal && isInCurrent) {
            list.removeAll(movieId);
            changed = true;
        }
    };

    updateList(watchlist, "watchlist");
    updateList(favorites, "favorites");
    updateList(history, "watchedHistory");
    updatePayload["watchlist"] = watchlist;
    updatePayload["favorites"] = favorites;
    updatePayload["watchedHistory"] = history;

    QStringList customListNames = customLists.keys();
    for(const QString& listName : customListNames) {
        QVariantList list = customLists[listName].toList();
        updateList(list, listName);
        customLists[listName] = list;
    }
    updatePayload["customLists"] = customLists;


    if (changed) {
        m_firestoreService->updateUserDocument(m_uid, m_idToken, updatePayload);
    }
}

QStringList UserManager::getAllListNames(bool includePredefined, bool includeCustom) const
{
    QStringList names;
    if (includePredefined) {
        names << "watchlist" << "favorites" << "watchedHistory";
    }
    if (includeCustom) {
        QVariantMap customLists = m_profileData["customLists"].toVariant().toMap();
        names << customLists.keys();
    }
    return names;
}

QStringList UserManager::getListsForMovie(int movieId) const
{
    QStringList lists;
    if (!m_isAuthenticated) return lists;

    if (m_profileData["watchlist"].toArray().contains(QJsonValue(movieId))) {
        lists << "watchlist";
    }
    if (m_profileData["favorites"].toArray().contains(QJsonValue(movieId))) {
        lists << "favorites";
    }
    if (m_profileData["watchedHistory"].toArray().contains(QJsonValue(movieId))) {
        lists << "watchedHistory";
    }

    QJsonObject customListsObj = m_profileData["customLists"].toObject();
    for (auto it = customListsObj.constBegin(); it != customListsObj.constEnd(); ++it) {
        if (it.value().toArray().contains(QJsonValue(movieId))) {
            lists << it.key();
        }
    }
    return lists;
}


bool UserManager::isMovieInList(int movieId, const QString &listName) const
{
    if (listName.isEmpty() || !m_isAuthenticated) return false;

    if (listName == "watchlist" || listName == "favorites" || listName == "watchedHistory") {
        return m_profileData[listName].toArray().contains(QJsonValue(movieId));
    } else {
        QJsonObject customLists = m_profileData["customLists"].toObject();
        if (customLists.contains(listName)) {
            return customLists[listName].toArray().contains(QJsonValue(movieId));
        }
    }
    return false;
}

QString UserManager::getUid() const {
    return m_uid;
}

QString UserManager::getIdToken() const {
    return m_idToken;
}

void UserManager::updateIdToken(const QString &newIdToken)
{
    if (m_idToken != newIdToken && !newIdToken.isEmpty()) {
        m_idToken = newIdToken;
        emit idTokenRefreshed(m_idToken);
    }
}

QString UserManager::getRefreshToken() const {
    return m_refreshToken;
}

QString UserManager::getEmail() const {
    return m_email;
}

void UserManager::setEmail(const QString &email)
{
    if (m_email != email) {
        m_email = email;
        emit emailChanged(m_email);
    }
}

QString UserManager::getDisplayName() const {
    return m_displayName;
}

void UserManager::setDisplayName(const QString &displayName)
{
    if (m_displayName != displayName) {
        m_displayName = displayName;
        emit displayNameChanged(m_displayName);
    }
}

QString UserManager::getProfilePictureUrl() const {
    return m_profilePictureLocalPath;
}

void UserManager::setProfilePictureUrl(const QString &localPath)
{
    if (m_profilePictureLocalPath != localPath) {
        m_profilePictureLocalPath = localPath;
        emit profilePictureChanged(m_profilePictureLocalPath);
    }
}

bool UserManager::isAuthenticated() const {
    return m_isAuthenticated;
}

bool UserManager::isGoogleSignIn() const {
    return m_isGoogleSignIn;
}
