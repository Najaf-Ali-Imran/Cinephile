#include "FirestoreService.h"
#include "usermanager.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

const QString FIREBASE_PROJECT_ID = "cinephile-971a8";

FirestoreService::FirestoreService(const QString &firebaseApiKey, QObject *parent)
    : QObject(parent)
    , m_projectId(FIREBASE_PROJECT_ID)
    , m_apiKey(firebaseApiKey)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager,
            &QNetworkAccessManager::finished,
            this,
            &FirestoreService::onNetworkReply);
    m_baseFirestoreUrl
        = QString("https://firestore.googleapis.com/v1/projects/%1/databases/(default)/documents")
              .arg(m_projectId);
}

QString FirestoreService::buildFirestoreUrl(const QString &path) const
{
    if (path.isEmpty()) {
        return m_baseFirestoreUrl;
    }
    return m_baseFirestoreUrl + "/" + path;
}

QString FirestoreService::buildFirestorePatchUrl(const QString &documentPath,
                                                 const QVariantMap &fieldsToUpdate) const
{
    QUrl url(buildFirestoreUrl(documentPath));
    QUrlQuery query;
    for (auto it = fieldsToUpdate.constBegin(); it != fieldsToUpdate.constEnd(); ++it) {
        query.addQueryItem("updateMask.fieldPaths", it.key());
    }
    url.setQuery(query);
    return url.toString();
}

void FirestoreService::sendFirestoreRequest(const QString &fullPath,
                                            const QString &method,
                                            const QString &idToken,
                                            const QByteArray &payload,
                                            const PendingOperation &opDetails)
{
    QUrl url(fullPath); 

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + idToken).toUtf8());
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = nullptr;
    QString upperMethod = method.toUpper();

    if (upperMethod == "GET") {
        reply = m_networkManager->get(request);
    } else if (upperMethod == "POST") {
        reply = m_networkManager->post(request, payload);
    } else if (upperMethod == "PUT") {
        reply = m_networkManager->put(request, payload);
    } else if (upperMethod == "PATCH") {
        reply = m_networkManager->sendCustomRequest(request, "PATCH", payload);
    } else if (upperMethod == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    } else {
        if (opDetails.type == FirestoreOperation::CheckUserProfile
            || opDetails.type == FirestoreOperation::CreateUserProfile) {
            emit userProfileCheckCreateComplete(false, opDetails.uid, {}, "Unsupported HTTP method");
        } else if (opDetails.type == FirestoreOperation::FetchUserProfile) {
            emit userProfileFetched(false, opDetails.uid, {}, "Unsupported HTTP method");
        } else if (opDetails.type == FirestoreOperation::UpdateUserDocument) {
            emit userDocumentUpdated(false, opDetails.uid, {}, "Unsupported HTTP method");
        }
        return;
    }

    if (reply) {
        m_pendingOperations[reply] = opDetails;
        if (!payload.isEmpty() && upperMethod != "GET") {
            qDebug() << "FirestoreService: Payload:" << QString(payload).left(200);
        }
    }
}

void FirestoreService::checkAndCreateUserProfile(const QString &uid,
                                                 const QString &idToken,
                                                 const QString &email,
                                                 const QString &displayName)
{
    QString userDocPath = "users/" + uid;
    PendingOperation opDetails;
    opDetails.type = FirestoreOperation::CheckUserProfile;
    opDetails.uid = uid;
    opDetails.idToken = idToken;
    opDetails.email = email;
    opDetails.displayName = displayName;

    sendFirestoreRequest(buildFirestoreUrl(userDocPath), "GET", idToken, QByteArray(), opDetails);
}

void FirestoreService::fetchUserProfile(const QString &uid, const QString &idToken)
{
    QString userDocPath = "users/" + uid;
    PendingOperation opDetails;
    opDetails.type = FirestoreOperation::FetchUserProfile;
    opDetails.uid = uid;
    opDetails.idToken = idToken;
    sendFirestoreRequest(buildFirestoreUrl(userDocPath), "GET", idToken, QByteArray(), opDetails);
}

void FirestoreService::updateUserDocument(const QString &uid,
                                          const QString &idToken,
                                          const QVariantMap &fieldsToUpdate)
{
    if (uid.isEmpty() || idToken.isEmpty() || fieldsToUpdate.isEmpty()) {
        qWarning()
        << "FirestoreService::updateUserDocument: Missing UID, ID Token, or fields to update.";
        emit userDocumentUpdated(false, uid, {}, "Invalid parameters for update.");
        return;
    }

    QString userDocPath = "users/" + uid;
    QString patchUrl = buildFirestorePatchUrl(userDocPath, fieldsToUpdate);

    QJsonObject firestorePayload;
    firestorePayload["fields"] = fieldsToJson(fieldsToUpdate);

    PendingOperation opDetails;
    opDetails.type = FirestoreOperation::UpdateUserDocument;
    opDetails.uid = uid;
    opDetails.idToken = idToken;
    opDetails.fieldsToUpdate = fieldsToUpdate;
    opDetails.originalPayload = jsonToPlainObject(
        firestorePayload);

    sendFirestoreRequest(patchUrl,
                         "PATCH",
                         idToken,
                         QJsonDocument(firestorePayload).toJson(),
                         opDetails);
}

QJsonObject valueToJson(const QVariant &value)
{
    QJsonObject valObj;
    QMetaType::Type typeId = static_cast<QMetaType::Type>(value.typeId());

    if (value.isNull() || !value.isValid()) {
        valObj["nullValue"] = QJsonValue::Null;
    } else if (typeId == QMetaType::QString) {
        valObj["stringValue"] = value.toString();
    } else if (typeId == QMetaType::LongLong || typeId == QMetaType::ULongLong
               || typeId == QMetaType::Int || typeId == QMetaType::UInt) {
        valObj["integerValue"]
            = value.toString();
    } else if (typeId == QMetaType::Double || typeId == QMetaType::Float) {
        valObj["doubleValue"] = value.toDouble();
    } else if (typeId == QMetaType::Bool) {
        valObj["booleanValue"] = value.toBool();
    } else if (typeId == QMetaType::QDateTime) {
        valObj["timestampValue"] = value.toDateTime().toUTC().toString(Qt::ISODateWithMs);
    } else if (value.canConvert<QVariantList>()) {
        QJsonArray valuesArray;
        for (const QVariant &v : value.toList()) {
            valuesArray.append(valueToJson(v));
        }
        valObj["arrayValue"] = QJsonObject{{"values", valuesArray}};
    } else if (value.canConvert<QVariantMap>()) {
        QJsonObject mapFieldsObj;
        QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            mapFieldsObj[it.key()] = valueToJson(it.value());
        }
        valObj["mapValue"] = QJsonObject{{"fields", mapFieldsObj}};
    } else {
        qWarning() << "valueToJson: Unhandled QVariant type:" << value.typeName() << "ID:" << typeId
                   << "Value:" << value;
        valObj["stringValue"] = value.toString();
    }
    return valObj;
}

QJsonObject fieldsToJson(const QVariantMap &fieldsMap)
{
    QJsonObject fieldsObj;
    for (auto it = fieldsMap.constBegin(); it != fieldsMap.constEnd(); ++it) {
        fieldsObj[it.key()] = valueToJson(it.value());
    }
    return fieldsObj;
}

QVariant jsonToValue(const QJsonObject &valueObject)
{
    if (valueObject.contains("stringValue")) {
        return valueObject["stringValue"].toString();
    } else if (valueObject.contains("integerValue")) {
        bool ok;
        qlonglong val = valueObject["integerValue"].toString().toLongLong(&ok);
        if (ok)
            return val;
        return valueObject["integerValue"].toVariant();
    } else if (valueObject.contains("doubleValue")) {
        return valueObject["doubleValue"].toDouble();
    } else if (valueObject.contains("booleanValue")) {
        return valueObject["booleanValue"].toBool();
    } else if (valueObject.contains("timestampValue")) {
        return QDateTime::fromString(valueObject["timestampValue"].toString(), Qt::ISODateWithMs);
    } else if (valueObject.contains("arrayValue")) {
        QJsonObject arrayValObj = valueObject["arrayValue"].toObject();
        if (arrayValObj.contains("values") && arrayValObj["values"].isArray()) {
            QJsonArray values = arrayValObj["values"].toArray();
            QVariantList list;
            for (const QJsonValue &val : values) {
                if (val.isObject())
                    list.append(jsonToValue(val.toObject()));
            }
            return list;
        }
        return QVariantList();
    } else if (valueObject.contains("mapValue")) {
        QJsonObject mapValObj = valueObject["mapValue"].toObject();
        if (mapValObj.contains("fields") && mapValObj["fields"].isObject()) {
            QJsonObject mapFields = mapValObj["fields"].toObject();
            QVariantMap map;
            for (auto it = mapFields.constBegin(); it != mapFields.constEnd(); ++it) {
                map[it.key()] = jsonToValue(it.value().toObject());
            }
            return map;
        }
        return QVariantMap();
    } else if (valueObject.contains("nullValue")) {
        return QVariant();
    }
    qWarning() << "jsonToValue: Unhandled Firestore value type in object:" << valueObject.keys();
    return QVariant();
}

QJsonObject jsonToPlainObject(const QJsonObject &firestoreDoc)
{
    QJsonObject plainObj;
    if (firestoreDoc.contains("fields") && firestoreDoc["fields"].isObject()) {
        QJsonObject fields = firestoreDoc["fields"].toObject();
        for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
            if (it.value().isObject()) {
                QVariant variantValue = jsonToValue(it.value().toObject());
                plainObj[it.key()] = QJsonValue::fromVariant(variantValue);
            } else {
                qWarning() << "jsonToPlainObject: Field" << it.key()
                << "does not have an object value:" << it.value();
            }
        }
    }
    return plainObj;
}

void FirestoreService::onNetworkReply(QNetworkReply *reply)
{
    if (!m_pendingOperations.contains(reply)) {
        qWarning() << "FirestoreService: Received reply for an unknown operation.";
        reply->deleteLater();
        return;
    }

    PendingOperation opDetails = m_pendingOperations.take(reply);
    QByteArray responseData = reply->readAll();
    QNetworkReply::NetworkError networkError = reply->error();
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString errorString = reply->errorString();

    qDebug() << "FirestoreService: Reply for OpType" << static_cast<int>(opDetails.type)
             << "UID:" << opDetails.uid << "HTTP" << httpStatusCode << "Error:" << networkError
             << "ErrorStr:" << errorString;
    if (!responseData.isEmpty()) {
        qDebug() << "FirestoreService: Response Data:" << QString(responseData).left(500);
    }

    if (opDetails.type == FirestoreOperation::CheckUserProfile && httpStatusCode == 404) {
        qDebug() << "FirestoreService: User profile NOT found for UID:" << opDetails.uid
                 << ". Proceeding to create...";

        QVariantMap profileFields;
        profileFields["email"] = opDetails.email;
        if (!opDetails.displayName.isEmpty()) {
            profileFields["displayName"] = opDetails.displayName;
        } else {
            profileFields["displayName"] = opDetails.email.split("@").first();
        }
        profileFields["createdAt"] = QDateTime::currentDateTimeUtc();
        profileFields["watchlist"] = QVariantList();
        profileFields["favorites"] = QVariantList();
        profileFields["watchedHistory"] = QVariantList();
        profileFields["customLists"] = QVariantMap();
        profileFields["profilePictureLocalPath"] = "";

        QJsonObject firestorePayload;
        firestorePayload["fields"] = fieldsToJson(profileFields);

        PendingOperation createOpDetails;
        createOpDetails.type = FirestoreOperation::CreateUserProfile;
        createOpDetails.uid = opDetails.uid;
        createOpDetails.idToken = opDetails.idToken;
        createOpDetails.email = opDetails.email;
        createOpDetails.displayName = profileFields["displayName"].toString();
        createOpDetails.originalPayload = jsonToPlainObject(firestorePayload);

        QString parentPath = "users";
        QString createUrlString = buildFirestoreUrl(parentPath);
        QUrl createUrlWithQuery(createUrlString);
        QUrlQuery query;
        query.addQueryItem("documentId", opDetails.uid);
        createUrlWithQuery.setQuery(query);

        sendFirestoreRequest(createUrlWithQuery.toString(),
                             "POST",
                             opDetails.idToken,
                             QJsonDocument(firestorePayload).toJson(),
                             createOpDetails);
        reply->deleteLater();
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        QString errorMessage = QString("Network error (%1): %2. URL: %3")
        .arg(networkError)
            .arg(errorString)
            .arg(reply->request().url().toString().left(100));
        if (opDetails.type == FirestoreOperation::CheckUserProfile
            || opDetails.type == FirestoreOperation::CreateUserProfile) {
            emit userProfileCheckCreateComplete(false, opDetails.uid, {}, errorMessage);
        } else if (opDetails.type == FirestoreOperation::FetchUserProfile) {
            emit userProfileFetched(false, opDetails.uid, {}, errorMessage);
        } else if (opDetails.type == FirestoreOperation::UpdateUserDocument) {
            emit userDocumentUpdated(false, opDetails.uid, {}, errorMessage);
        }
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonResponse;
    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        jsonResponse = jsonDoc.object();
    }

    if (jsonResponse.contains("error")) {
        QString errorMessage = jsonResponse["error"].toObject()["message"].toString(
            "Unknown Firestore error from response body.");
        qWarning() << "FirestoreService: Error in JSON response for OpType"
                   << static_cast<int>(opDetails.type) << ":" << errorMessage;
        if (opDetails.type == FirestoreOperation::CheckUserProfile
            || opDetails.type == FirestoreOperation::CreateUserProfile) {
            emit userProfileCheckCreateComplete(false, opDetails.uid, {}, errorMessage);
        } else if (opDetails.type == FirestoreOperation::FetchUserProfile) {
            emit userProfileFetched(false, opDetails.uid, {}, errorMessage);
        } else if (opDetails.type == FirestoreOperation::UpdateUserDocument) {
            emit userDocumentUpdated(false, opDetails.uid, {}, errorMessage);
        }
        reply->deleteLater();
        return;
    }

    switch (opDetails.type) {
    case FirestoreOperation::CheckUserProfile: {
        if (httpStatusCode == 200) {
            qDebug() << "FirestoreService: User profile exists for UID:" << opDetails.uid;
            QJsonObject plainProfileData = jsonToPlainObject(jsonResponse);

            UserManager::instance()->setProfileData(plainProfileData);

            emit userProfileCheckCreateComplete(true, opDetails.uid, plainProfileData, QString());
        } else {
            emit userProfileCheckCreateComplete(false,
                                                opDetails.uid,
                                                {},
                                                "Unexpected HTTP status "
                                                    + QString::number(httpStatusCode)
                                                    + " for CheckUserProfile.");
        }
        break;
    }
    case FirestoreOperation::CreateUserProfile: {
        if (httpStatusCode == 200) {
            QJsonObject createdProfileData = jsonToPlainObject(jsonResponse);

            UserManager::instance()->setProfileData(createdProfileData);

            emit userProfileCheckCreateComplete(true, opDetails.uid, createdProfileData, QString());
        } else {
            emit userProfileCheckCreateComplete(false,
                                                opDetails.uid,
                                                {},
                                                "Error creating profile: HTTP "
                                                    + QString::number(httpStatusCode) + " "
                                                    + responseData);
        }
        break;
    }
    case FirestoreOperation::FetchUserProfile: {
        if (httpStatusCode == 200) {
            QJsonObject plainProfileData = jsonToPlainObject(jsonResponse);

            UserManager::instance()->setProfileData(plainProfileData);

            emit userProfileFetched(true, opDetails.uid, plainProfileData, QString());
        } else if (httpStatusCode == 404) {
            emit userProfileFetched(false,
                                    opDetails.uid,
                                    {},
                                    "Profile not found during FetchUserProfile.");
        } else {
            emit userProfileFetched(false,
                                    opDetails.uid,
                                    {},
                                    "Error fetching profile: HTTP "
                                        + QString::number(httpStatusCode));
        }
        break;
    }
    case FirestoreOperation::UpdateUserDocument: {
        if (httpStatusCode == 200) {
            QJsonObject updatedData = jsonToPlainObject(
                jsonResponse);

            UserManager::instance()->setProfileData(updatedData);

            QJsonObject intendedUpdatesPlain;
            for (auto it = opDetails.fieldsToUpdate.constBegin();
                 it != opDetails.fieldsToUpdate.constEnd();
                 ++it) {
                intendedUpdatesPlain[it.key()] = QJsonValue::fromVariant(it.value());
            }
            emit userDocumentUpdated(true, opDetails.uid, intendedUpdatesPlain, QString());
        } else {
            emit userDocumentUpdated(false,
                                     opDetails.uid,
                                     {},
                                     "Error updating document: HTTP "
                                         + QString::number(httpStatusCode) + " " + responseData);
        }
        break;
    }
    default:
        qWarning() << "FirestoreService: Unhandled operation type in reply (after error checks):"
                   << static_cast<int>(opDetails.type);
        break;
    }
    reply->deleteLater();
}
