#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Fontschau");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("Fontschau");

    MainWindow window;
    window.show();

    return app.exec();
}