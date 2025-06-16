#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QScreen>
#include <QStyle>
#include "FirestoreService.h"
#include "LoginWidget.h"
#include "MainWindow.h"
#include "UserManager.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginWidget loginWidget;

    FirestoreService firestoreService(
        loginWidget.property("m_firebaseApiKey").toString());

    MainWindow mainWindow(&loginWidget, &firestoreService);

    loginWidget.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    loginWidget.setAutoFillBackground(false);
    loginWidget.setAttribute(Qt::WA_NoSystemBackground, true);
    loginWidget.resize(850, 600);
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        loginWidget.setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                                    Qt::AlignCenter,
                                                    loginWidget.size(),
                                                    screenGeometry));
    }

    QObject::connect(&loginWidget,
                     &LoginWidget::userAuthenticated,
                     [&](const QString &uid,
                         const QString &idToken,
                         const QString &refreshToken,
                         const QString &email,
                         const QString &displayName) {
                         qDebug() << "User authenticated via LoginWidget signal. UID:" << uid;
                         mainWindow.show();
                         loginWidget.close();
                     });

    QObject::connect(&loginWidget, &LoginWidget::closeApp, &app, &QApplication::quit);
    QObject::connect(&loginWidget, &LoginWidget::minimizeApp, &loginWidget, &QWidget::showMinimized);

    QObject::connect(&mainWindow, &MainWindow::aboutToLogout, [&]() {
        qDebug() << "MainWindow signaled aboutToLogout. Switching to LoginWidget.";
        mainWindow.hide();
        loginWidget.clearLoginPassword();
        loginWidget.show();
    });

    loginWidget.show();

    return app.exec();
}
