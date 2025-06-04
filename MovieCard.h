#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include "Theme.h" // Make sure AppTheme members are defined here

class MovieCard : public QWidget {
    Q_OBJECT
public:
    explicit MovieCard(const QJsonObject &itemData,
                       const QString &imageBaseUrl,
                       const QString &posterSize,
                       QWidget *parent = nullptr);

    // Add these signals
signals:
    void clicked(const QJsonObject &itemData);

protected:
    // Add this to handle mouse clicks
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUI(const QJsonObject &itemData);
    void loadPoster(const QString &posterPath);

    QNetworkAccessManager *m_networkManager;
    QString m_imageBaseUrl;
    QString m_posterSize;
    QJsonObject m_itemData;  // Add this to store the item data
};
