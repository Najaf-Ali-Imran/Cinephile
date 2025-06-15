#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QWidget>
#include "Theme.h"

class MovieCard : public QWidget
{
    Q_OBJECT
public:
    explicit MovieCard(const QJsonObject &itemData,
                       const QString &imageBaseUrl,
                       const QString &posterSize,
                       QWidget *parent = nullptr);

signals:
    void clicked(const QJsonObject &itemData);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUI(const QJsonObject &itemData);
    void loadPoster(const QString &posterPath);

    QNetworkAccessManager *m_networkManager;
    QString m_imageBaseUrl;
    QString m_posterSize;
    QJsonObject m_itemData;
};
