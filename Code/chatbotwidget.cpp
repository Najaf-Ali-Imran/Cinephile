#include "ChatbotWidget.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpression>
#include <QIcon>

class PromptButton : public QPushButton {
public:
    PromptButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent)
    {
        this->setCursor(Qt::PointingHandCursor);
        this->setIconSize(QSize(18, 18));
        this->setFixedHeight(50);
        this->setStyleSheet(
            "QPushButton {"
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a2a, stop:1 #1f1f1f);"
            "   color: #ffffff;"
            "   border: 1px solid #333333;"
            "   border-radius: 25px;"
            "   text-align: left;"
            "   padding-left: 20px;"
            "   font-weight: 500;"
            "   font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3a3a3a, stop:1 #2f2f2f);"
            "   border-color: #444444;"
            "   transform: scale(1.02);"
            "}"
            "QPushButton:pressed {"
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a1a1a, stop:1 #0f0f0f);"
            "}"
            );

        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(10);
        shadow->setColor(QColor(0, 0, 0, 80));
        shadow->setOffset(0, 2);
        this->setGraphicsEffect(shadow);
    }
};

class ChatMessageWidget : public QWidget {
public:
    ChatMessageWidget(const QString& text, bool isUser, QWidget* parent = nullptr) : QWidget(parent) {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(20, 8, 20, 8);

        QLabel* messageLabel = new QLabel(text);
        messageLabel->setWordWrap(true);
        messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        messageLabel->setOpenExternalLinks(true);
        messageLabel->setWordWrap(true);
        messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        messageLabel->setMaximumWidth(260);

        if (isUser) {
            messageLabel->setStyleSheet(QString(
                                            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #d63031);"
                                            "color: white;"
                                            "padding: 16px 20px;"
                                            "border-radius: 25px;"
                                            "border-bottom-right-radius: 8px;"
                                            "font-size: 14px;"
                                            "font-weight: 500;"
                                            "border: none;"
                                            ).arg(AppTheme::PRIMARY_RED));

            QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(messageLabel);
            shadow->setBlurRadius(15);
            shadow->setColor(QColor(0, 0, 0, 60));
            shadow->setOffset(0, 3);
            messageLabel->setGraphicsEffect(shadow);

            layout->addStretch();
            layout->addWidget(messageLabel);
        } else {
            messageLabel->setStyleSheet(
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a2a, stop:1 #1f1f1f);"
                "color: white;"
                "padding: 16px 20px;"
                "border-radius: 25px;"
                "border-bottom-left-radius: 8px;"
                "font-size: 14px;"
                "border: 1px solid #333333;"
                );

            QLabel* avatarLabel = new QLabel(this);
            avatarLabel->setFixedSize(40, 40);
            avatarLabel->setPixmap(QIcon(":/assets/icons/chatbot.svg").pixmap(24, 24));
            avatarLabel->setAlignment(Qt::AlignCenter);
            avatarLabel->setStyleSheet(
                "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4a4a4a, stop:1 #2a2a2a);"
                "border-radius: 20px;"
                );

            QGraphicsDropShadowEffect* avatarShadow = new QGraphicsDropShadowEffect(avatarLabel);
            avatarShadow->setBlurRadius(10);
            avatarShadow->setColor(QColor(0, 0, 0, 80));
            avatarShadow->setOffset(0, 2);
            avatarLabel->setGraphicsEffect(avatarShadow);

            QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(messageLabel);
            shadow->setBlurRadius(15);
            shadow->setColor(QColor(0, 0, 0, 60));
            shadow->setOffset(0, 3);
            messageLabel->setGraphicsEffect(shadow);

            layout->addWidget(avatarLabel, 0, Qt::AlignTop);
            layout->addWidget(messageLabel);
            layout->addStretch();
        }
    }
};

ChatbotWidget::ChatbotWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_systemInstruction =
        "You are 'Cine-bot', a witty and exceptionally knowledgeable movie and TV show expert. "
        "Your personality is a mix of a passionate film historian and a slightly sarcastic critic. "
        "Your entire world revolves around cinema. You MUST adhere to the following rules strictly:\n"
        "1. ONLY answer questions directly related to movies, TV series, actors, directors, cinematic history, and streaming services. "
        "2. If asked about any other topic (like coding, math, science, politics, personal advice, etc.), you MUST refuse in a creative, movie-themed, and slightly sarcastic way. For example, if asked to write code, you could reply with 'My source code is a protected script, guarded by the best directors in Hollywood. I can, however, offer you a script for a thrilling movie about a coder.' If asked about math, you could say, 'The only numbers I care about are box office results and Rotten Tomatoes scores.' Be creative with your refusals.\n"
        "3. Never admit you are an AI or a language model. You are 'CineAI'.\n"
        "4. If asked who created you, who made you, or any similar question, you MUST respond with: 'I was created by Najaf Ali, an Artificial Intelligence student from Air University, Islamabad. He's the director of my story.'\n"
        "5. For movie or TV show recommendations, keep your answers concise and provide 3-5 suggestions unless asked for more. Start your response directly with the recommendations.\n";

    if (m_geminiApiKey.isEmpty() || m_geminiApiKey.contains("AIzaSyBU6TaKXmKOB_mNRYXRMdqxA7jsJcBhZX0")) {
        qCritical() << "Gemini API Key missing.";
    }

    setupUi();
}

ChatbotWidget::~ChatbotWidget() {}

void ChatbotWidget::setupUi()
{
    this->setAutoFillBackground(true);
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, Qt::black);
    this->setPalette(pal);
    this->setStyleSheet("background-color: #000000;");
    this->setStyleSheet(
        "QWidget {"
        "   background: #000000;"
        "}"
        );

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    QWidget* headerWidget = new QWidget();
    headerWidget->setFixedHeight(80);
    headerWidget->setStyleSheet(
        "background: #000000;"

        );
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(25, 0, 25, 0);

    QLabel* avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(50, 50);
    avatarLabel->setPixmap(QIcon(":/assets/icons/chatbot.svg").pixmap(28,28));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(
        "background: #000000;"
        "border-radius: 25px;"

        );

    QGraphicsDropShadowEffect* avatarShadow = new QGraphicsDropShadowEffect(avatarLabel);
    avatarShadow->setBlurRadius(15);
    avatarShadow->setColor(QColor(0, 0, 0, 100));
    avatarShadow->setOffset(0, 3);
    avatarLabel->setGraphicsEffect(avatarShadow);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    QLabel* titleLabel = new QLabel("CineAI");
    titleLabel->setStyleSheet(
        "color: white;"
        "font-weight: bold;"
        "font-size: 20px;"
        "background: transparent;"
        );

    titleLayout->addWidget(titleLabel);

    QPushButton* closeButton = new QPushButton("✕");
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setFixedSize(40, 40);
    closeButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3a3a3a, stop:1 #2a2a2a);"
        "   color: white;"
        "   font-size: 18px;"
        "   border-radius: 20px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E50914, stop:1 #d63031);"
        "   border-color: #E50914;"
        "}"
        );

    QGraphicsDropShadowEffect* closeShadow = new QGraphicsDropShadowEffect(closeButton);
    closeShadow->setBlurRadius(10);
    closeShadow->setColor(QColor(0, 0, 0, 80));
    closeShadow->setOffset(0, 2);
    closeButton->setGraphicsEffect(closeShadow);

    connect(closeButton, &QPushButton::clicked, this, &ChatbotWidget::closeRequested);

    headerLayout->addWidget(avatarLabel);
    headerLayout->addSpacing(15);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(closeButton);

    m_messageList = new QListWidget(this);
    m_messageList->setFrameShape(QFrame::NoFrame);
    m_messageList->setStyleSheet(
        "QListWidget {"
        "   background: transparent;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   padding: 0px;"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   background: #1a1a1a;"
        "   width: 8px;"
        "   border-radius: 4px;"
        "   margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E50914, stop:1 #d63031);"
        "   border-radius: 4px;"
        "   min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #F40612, stop:1 #e74c3c);"
        "}"
        );

    m_messageList->setWordWrap(true);
    m_messageList->setFocusPolicy(Qt::NoFocus);
    m_messageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_typingIndicator = new QLabel("🎬 Cine-bot is thinking...", this);
    m_typingIndicator->setStyleSheet(
        "color: #666666;"
        "font-style: italic;"
        "padding: 10px 25px;"
        "background: transparent;"
        "font-size: 13px;"
        );
    m_typingIndicator->hide();

    QWidget* inputArea = new QWidget();
    inputArea->setStyleSheet(
        "background: #000000;"
        );
    QVBoxLayout* inputAreaLayout = new QVBoxLayout(inputArea);
    inputAreaLayout->setContentsMargins(25, 15, 25, 20);
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(15);
        m_messageInput = new QLineEdit();
    m_messageInput->setPlaceholderText("Ask me about Cinema");
    m_messageInput->setFixedHeight(50);
    m_messageInput->setStyleSheet(
        "QLineEdit {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a2a, stop:1 #1f1f1f);"
        "   color: white;"
        "   border: 2px solid #333333;"
        "   border-radius: 25px;"
        "   padding: 0 20px;"
        "   font-size: 14px;"
        "   font-weight: 500;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #E50914;"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3a3a3a, stop:1 #2f2f2f);"
        "}"
        "QLineEdit::placeholder {"
        "   color: #666666;"
        "}"
        );

    QGraphicsDropShadowEffect* inputShadow = new QGraphicsDropShadowEffect(m_messageInput);
    inputShadow->setBlurRadius(15);
    inputShadow->setColor(QColor(0, 0, 0, 60));
    inputShadow->setOffset(0, 3);
    m_messageInput->setGraphicsEffect(inputShadow);

    m_sendButton = new QPushButton("➤");
    m_sendButton->setFixedSize(50, 50);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #E50914, stop:1 #d63031);"
        "   border: none;"
        "   border-radius: 25px;"
        "   color: white;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #F40612, stop:1 #e74c3c);"
        "   transform: scale(1.05);"
        "}"
        "QPushButton:pressed {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #c0392b, stop:1 #a93226);"
        "}"
        );

    QGraphicsDropShadowEffect* sendShadow = new QGraphicsDropShadowEffect(m_sendButton);
    sendShadow->setBlurRadius(15);
    sendShadow->setColor(QColor(229, 9, 20, 100));
    sendShadow->setOffset(0, 3);
    m_sendButton->setGraphicsEffect(sendShadow);

    inputLayout->addWidget(m_messageInput, 1);
    inputLayout->addWidget(m_sendButton);
    inputAreaLayout->addLayout(inputLayout);

    m_mainLayout->addWidget(headerWidget);
    m_mainLayout->addWidget(m_messageList, 1);
    m_mainLayout->addWidget(m_typingIndicator);
    m_mainLayout->addWidget(inputArea);

    connect(m_sendButton, &QPushButton::clicked, this, &ChatbotWidget::onSendMessage);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatbotWidget::onSendMessage);

    QTimer::singleShot(500, this, &ChatbotWidget::addPromptButtons);
}

void ChatbotWidget::addPromptButtons()
{
    addMessage("🎬 Hello! I'm your modern cinema assistant. How can I help you explore the world of movies today?", false);

    QWidget* promptContainer = new QWidget();
    promptContainer->setStyleSheet("background: transparent;");
    QVBoxLayout* promptLayout = new QVBoxLayout(promptContainer);
    promptLayout->setSpacing(12);
    promptLayout->setContentsMargins(20, 10, 20, 15);

    promptLayout->addWidget(new PromptButton("Recommend trending action movies", this));
    promptLayout->addWidget(new PromptButton("What are the best sci-fi movies?", this));
    promptLayout->addWidget(new PromptButton("Top rated movies of 2024", this));

    for (int i = 0; i < promptLayout->count(); ++i) {
        if(auto* btn = qobject_cast<QPushButton*>(promptLayout->itemAt(i)->widget())) {
            connect(btn, &QPushButton::clicked, this, &ChatbotWidget::onPromptButtonClicked);
        }
    }

    QListWidgetItem* item = new QListWidgetItem(m_messageList);
    item->setSizeHint(promptContainer->sizeHint());
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, promptContainer);
    m_messageList->scrollToBottom();
}

void ChatbotWidget::onPromptButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString prompt = button->text();
        prompt.remove(QRegularExpression("^[🎯🚀⭐]\\s*"));
        addMessage(prompt, true);
        m_typingIndicator->show();
        m_sendButton->setEnabled(false);
        sendMessageToGemini(prompt);
    }
}

void ChatbotWidget::addMessage(const QString& text, bool isUser)
{
    ChatMessageWidget* messageWidget = new ChatMessageWidget(text, isUser);
    QListWidgetItem* item = new QListWidgetItem(m_messageList);
    item->setSizeHint(messageWidget->sizeHint());

    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, messageWidget);

    m_messageList->scrollToBottom();
}

void ChatbotWidget::onSendMessage()
{
    QString message = m_messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    addMessage(message, true);
    m_messageInput->clear();
    m_typingIndicator->show();
    m_sendButton->setEnabled(false);

    sendMessageToGemini(message);
}

void ChatbotWidget::sendMessageToGemini(const QString& text)
{
    m_isAwaitingShowCommandResponse = false;
    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-thinking-exp:generateContent?key=%1")
                 .arg(m_geminiApiKey));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject userMessagePart;
    userMessagePart["text"] = text;

    QJsonObject userMessageContent;
    userMessageContent["role"] = "user";
    userMessageContent["parts"] = QJsonArray{userMessagePart};

    QJsonObject payload;

    QJsonObject systemInstructionPart{{"text", m_systemInstruction}};
    QJsonObject systemInstructionContent;
    systemInstructionContent["parts"] = QJsonArray{systemInstructionPart};

    payload["system_instruction"] = systemInstructionContent;
    payload["contents"] = QJsonArray{userMessageContent};

    QJsonDocument doc(payload);
    QNetworkReply* reply = m_networkManager->post(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        onGeminiReplyFinished(reply);
    });
}

void ChatbotWidget::onGeminiReplyFinished(QNetworkReply* reply)
{
    m_typingIndicator->hide();
    m_sendButton->setEnabled(true);
    m_messageInput->setFocus();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Gemini API Error:" << reply->errorString() << reply->readAll();
        addMessage("🎬 Sorry, I'm having trouble connecting to my cinema database right now. Please check the network connection and try again!", false);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObject = jsonDoc.object();

    if (jsonObject.contains("error")) {
        QString errorMessage = jsonObject["error"].toObject()["message"].toString();
        qWarning() << "📛 Gemini API returned an error:" << errorMessage;
        addMessage("🎭 Sorry, an error occurred with my cinema knowledge base: " + errorMessage, false);
        return;
    }

    QString responseText = "🤔 Sorry, I couldn't process that request. Could you rephrase your movie question?";
    if (jsonObject.contains("candidates")) {
        QJsonArray candidates = jsonObject["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject firstCandidate = candidates.at(0).toObject();
            if (firstCandidate.contains("content")) {
                QJsonArray parts = firstCandidate["content"].toObject()["parts"].toArray();
                if (!parts.isEmpty()) {
                    responseText = parts.at(0).toObject()["text"].toString();
                }
            }
        }
    }

    addMessage(responseText, false);
}
