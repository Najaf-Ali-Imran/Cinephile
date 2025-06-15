#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDebug>

ConfigManager::ConfigManager(QObject *parent) : QObject(parent)
{
    loadConfig();
}

ConfigManager* ConfigManager::instance()
{
    static ConfigManager self;
    return &self;
}

void ConfigManager::loadConfig()
{
    QFile configFile(":/config.json");

    if (!configFile.exists()) {
        qFatal("CRITICAL: Embedded config.json not found in resource file.");
        return;
    }

    if (!configFile.open(QIODevice::ReadOnly)) {
        qFatal("CRITICAL: Could not open embedded config.json for reading.");
        return;
    }

    QJsonDocument configDoc = QJsonDocument::fromJson(configFile.readAll());
    if (configDoc.isNull() || !configDoc.isObject()) {
        qFatal("CRITICAL: Failed to parse embedded config.json. Make sure it is valid JSON.");
        return;
    }

    m_config = configDoc.object();
    qInfo() << "Configuration loaded successfully from resource.";
}


QString ConfigManager::getFirebaseApiKey() const
{
    return m_config["Firebase"].toObject()["ApiKey"].toString();
}

QString ConfigManager::getGoogleClientId() const
{
    return m_config["GoogleOAuth"].toObject()["ClientId"].toString();
}

QString ConfigManager::getGoogleClientSecret() const
{
    return m_config["GoogleOAuth"].toObject()["ClientSecret"].toString();
}



QString ConfigManager::getGeminiApiKey() const
{
    return m_config["Gemini"].toObject()["ApiKey"].toString();
}
