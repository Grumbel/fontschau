#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Fontora");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("Fontora");

    MainWindow window;
    window.show();

    return app.exec();
}