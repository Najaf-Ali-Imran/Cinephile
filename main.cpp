// --- START OF FILE main.cpp ---


#include "MainWindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);



    MainWindow mainWindow;

    mainWindow.updateDbStatusIcon(false);
    mainWindow.show();

    return app.exec();
}
// --- END OF FILE main.cpp ---
