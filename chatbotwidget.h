#ifndef CHATBOTWIDGET_H
#define CHATBOTWIDGET_H

#include <QWidget>
#include <QNetworkAccessManager>
#include "ConfigManager.h"

class QListWidget;
class QLineEdit;
class QPushButton;
class QNetworkReply;
class QVBoxLayout;
class QLabel;

class ChatbotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatbotWidget(QWidget *parent = nullptr);
    ~ChatbotWidget();

signals:
    void searchRequested(const QString& query);
    void closeRequested();

private slots:
    void onSendMessage();
    void onGeminiReplyFinished(QNetworkReply* reply);
    void onPromptButtonClicked();

private:
    void setupUi();
    void addMessage(const QString& text, bool isUser);
    void addPromptButtons();
    void sendMessageToGemini(const QString& text);

    QVBoxLayout* m_mainLayout;
    QListWidget* m_messageList;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;
    QLabel* m_typingIndicator;

    QNetworkAccessManager* m_networkManager;
    const QString m_geminiApiKey = ConfigManager::instance()->getGeminiApiKey();

    QString m_systemInstruction;
    bool m_isAwaitingShowCommandResponse = false;
};

#endif
