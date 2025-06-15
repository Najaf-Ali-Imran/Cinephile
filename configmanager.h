#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class ConfigManager : public QObject
{
    Q_OBJECT
private:
    explicit ConfigManager(QObject *parent = nullptr);
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

public:
    static ConfigManager* instance();

    QString getFirebaseApiKey() const;
    QString getGoogleClientId() const;
    QString getGoogleClientSecret() const;
    QString getGeminiApiKey() const;

private:
    void loadConfig();
    QJsonObject m_config;
};

#endif // CONFIGMANAGER_H
