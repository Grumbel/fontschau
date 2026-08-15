// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

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