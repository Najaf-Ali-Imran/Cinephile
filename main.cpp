// --- START OF FILE main.cpp ---

#include <QApplication>
#include <QFile>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;

    mainWindow.updateDbStatusIcon(true);
    mainWindow.show();

    return app.exec();
}
// --- END OF FILE main.cpp ---
