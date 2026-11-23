// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QIcon>

#include <cstdio>

#include "mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Fontschau - Qt font viewer"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption fontOption(
        {QStringLiteral("f"), QStringLiteral("font")},
        QStringLiteral("Load a font from a file (TTF, OTF, or collection)."),
        QStringLiteral("file"));
    parser.addOption(fontOption);
    parser.addPositionalArgument(
        QStringLiteral("font-file"),
        QStringLiteral("Font file to load at startup (alternative to --font)."),
        QStringLiteral("[font-file]"));

    // Handle help/version with QtCore only, so neither requires a display or
    // loading a Qt platform plugin. Only one application object is created.
    bool cliOnly = false;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("--help") || arg == QLatin1String("-h") ||
            arg == QLatin1String("--version") || arg == QLatin1String("-v")) {
            cliOnly = true;
            break;
        }
    }
    if (cliOnly) {
        QCoreApplication cliApp(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("fontschau"));
        QCoreApplication::setApplicationVersion(QString::fromLatin1(FONTSCHAU_VERSION));
        parser.process(cliApp); // Qt prints help/version and exits.
        return 0;
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Fontschau"));
    QApplication::setApplicationVersion(QString::fromLatin1(FONTSCHAU_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Fontschau"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/fontschau.svg")));
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.size() > 1 || (parser.isSet(fontOption) && !positional.isEmpty())) {
        std::fprintf(stderr, "fontschau: specify at most one font file\n");
        return 2;
    }
    const QString fontPath = parser.isSet(fontOption) ? parser.value(fontOption)
                                                       : positional.value(0);

    MainWindow window;
    if (!fontPath.isEmpty()) {
        QString error;
        if (!window.loadFontFile(fontPath, &error)) {
            std::fprintf(stderr, "fontschau: %s\n", qPrintable(error));
            return 1;
        }
    }
    window.show();

    return app.exec();
}
