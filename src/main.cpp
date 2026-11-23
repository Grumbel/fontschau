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
        QStringLiteral("Load a font file (TTF, OTF, or collection); repeat to compare two fonts."),
        QStringLiteral("file"));
    parser.addOption(fontOption);
    const QCommandLineOption rendererOption(
        QStringLiteral("renderer"),
        QStringLiteral("Initial renderer: qt or freetype (default: qt)."),
        QStringLiteral("engine"), QStringLiteral("qt"));
    parser.addOption(rendererOption);
    const QCommandLineOption splitOption(
        QStringLiteral("split"), QStringLiteral("Open two comparison panes."));
    parser.addOption(splitOption);
    parser.addPositionalArgument(
        QStringLiteral("font-file"),
        QStringLiteral("One or two font files to open. Two files automatically split the preview."),
        QStringLiteral("[font-file ...]"));

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

    // --font may be repeated, and positional paths work as before. When two
    // files are given the first belongs to pane A and the second to pane B.
    // Mixing forms is supported (option values precede positional arguments).
    QStringList fontPaths = parser.values(fontOption);
    fontPaths.append(parser.positionalArguments());
    if (fontPaths.size() > 2) {
        std::fprintf(stderr, "fontschau: specify at most two font files\n");
        return 2;
    }

    const QString engine = parser.value(rendererOption).toLower();
    if (engine != QLatin1String("qt") && engine != QLatin1String("freetype")) {
        std::fprintf(stderr, "fontschau: --renderer must be qt or freetype\n");
        return 2;
    }

    MainWindow window;
    if (engine == QLatin1String("freetype"))
        window.setPrimaryBackend(RenderBackend::FreeType);
    QString error;
    if (fontPaths.size() == 2) {
        if (!window.loadComparisonFonts(fontPaths.at(0), fontPaths.at(1), &error)) {
            std::fprintf(stderr, "fontschau: %s\n", qPrintable(error));
            return 1;
        }
    } else {
        if (fontPaths.size() == 1 && !window.loadFontFile(fontPaths.first(), &error)) {
            std::fprintf(stderr, "fontschau: %s\n", qPrintable(error));
            return 1;
        }
        if (parser.isSet(splitOption))
            window.setSplitPreview(true);
    }
    window.show();

    return app.exec();
}
