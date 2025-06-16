#ifndef RECOMMENDATIONPAGEWIDGET_H
#define RECOMMENDATIONPAGEWIDGET_H
#include "ChatbotWidget.h"
#include <QWidget>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QMap>
#include <QLabel>
#include <QHBoxLayout>


class QVBoxLayout;
class QGridLayout;
class QScrollArea;
class QLabel;
class QPushButton;
class QNetworkReply;
class MovieCard;

class RecommendationPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecommendationPageWidget(QNetworkAccessManager *networkManager, QWidget *parent = nullptr);
    ~RecommendationPageWidget();

    void generateRecommendations();

signals:
    void movieClicked(int id, const QString &type);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void processTasteProfileReply(QNetworkReply *reply);
    void processFinalRecommendationsReply(QNetworkReply *reply);
    void toggleChatbotPanel();

private:
    void setupUi();
    void setupChatbotButton();
    void analyzeTasteProfile(const QSet<int>& movieIds);
    void fetchTasteProfileDetails(int movieId);
    void discoverMoviesFromTaste();
    void clearGrid();
    void showStatusMessage(const QString& message);
    void updateHeader(const QString& title, const QString& subtitle);

    QNetworkAccessManager *m_networkManager;

    const QString m_apiKey = "b4c5ef5419f9f4ce8a627faa1e2be530";
    QPushButton* m_chatbotButton;
    QWidget* m_chatbotPanel;
    ChatbotWidget* m_chatbot;

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_resultsGrid;
    QLabel *m_statusLabel;

    int m_pendingDetailsRequests;
    QMap<int, int> m_genreScores;
    QMap<int, int> m_keywordScores;

    QList<MovieCard*> m_movieCards;
};

#endif
