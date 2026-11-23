// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QListWidget>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QSpinBox>
#include <QTextEdit>

#include <fontconfig/fontconfig.h>

namespace {
void click(QWidget *widget)
{
    const QPointF pos(2, 2);
    QMouseEvent press(QEvent::MouseButtonPress, pos,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, pos,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &release);
}

// Find actual installed font files without relying on a distribution-specific
// directory name. The comparison startup needs two valid paths to test that
// each pane retains the correct one.
QStringList comparisonFontFiles(bool sameFamily = false)
{
    QStringList result;
    QString firstFamily;
    auto *pattern = FcPatternCreate();
    auto *objects = FcObjectSetBuild(FC_FILE, FC_FAMILY, static_cast<char *>(nullptr));
    auto *fonts = FcFontList(nullptr, pattern, objects);
    if (fonts) {
        for (int i = 0; i < fonts->nfont; ++i) {
            FcChar8 *file = nullptr;
            FcChar8 *family = nullptr;
            if (FcPatternGetString(fonts->fonts[i], FC_FILE, 0, &file) != FcResultMatch ||
                FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &family) != FcResultMatch)
                continue;
            const QString path = QString::fromUtf8(reinterpret_cast<const char *>(file));
            const QString name = QString::fromUtf8(reinterpret_cast<const char *>(family));
            const QString ext = QFileInfo(path).suffix().toLower();
            if ((ext != QStringLiteral("ttf") && ext != QStringLiteral("otf") &&
                 ext != QStringLiteral("ttc") && ext != QStringLiteral("otc")) ||
                !QFileInfo(path).isReadable())
                continue;
            if (result.isEmpty()) {
                result << path;
                firstFamily = name;
            } else if (path != result.first() &&
                       (sameFamily ? name == firstFamily : name != firstFamily)) {
                result << path;
                break;
            }
        }
        FcFontSetDestroy(fonts);
    }
    FcObjectSetDestroy(objects);
    FcPatternDestroy(pattern);
    return result;
}

bool previewsAligned(PreviewPane *a, PreviewPane *b, QWidget *window)
{
    const auto *scrollA = a->findChild<QScrollArea *>(QStringLiteral("previewScroll"));
    const auto *scrollB = b->findChild<QScrollArea *>(QStringLiteral("previewScroll"));
    return scrollA && scrollB &&
           scrollA->mapTo(window, QPoint(0, 0)).y() ==
               scrollB->mapTo(window, QPoint(0, 0)).y();
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow window;
    auto *splitter = window.findChild<QSplitter *>(QStringLiteral("previewSplitter"));
    if (!splitter || splitter->count() != 1) return 1;
    // Default samples exercise Latin diacritics, several Cyrillic alphabets,
    // and emoji in both renderers (even if the chosen face lacks some glyphs).
    auto *sampleEdit = window.findChild<QTextEdit *>();
    if (!sampleEdit) return 21;
    const QString defaultSample = sampleEdit->toPlainText();
    if (!defaultSample.contains(QStringLiteral("Ää Öö Üü ß ẞ")) ||
        !defaultSample.contains(QStringLiteral("Åå Ææ")) ||
        !defaultSample.contains(QStringLiteral("Ąą Ćć Čč")) ||
        !defaultSample.contains(QStringLiteral("Șș Țț Őő Űű")) ||
        !defaultSample.contains(QStringLiteral("Съешь ещё этих мягких")) ||
        !defaultSample.contains(QStringLiteral("Її Єє Іі Ґґ")) ||
        !defaultSample.contains(QStringLiteral("😀 😃 😎"))) return 21;
    window.setPrimaryBackend(RenderBackend::FreeType);
    window.show();
    app.processEvents();

    auto *renderingGroup = window.findChild<QGroupBox *>(QStringLiteral("renderingGroup"));
    auto *freeTypeGroup = window.findChild<QGroupBox *>(QStringLiteral("freetypeGroup"));
    if (!renderingGroup || !freeTypeGroup || !freeTypeGroup->isVisible() ||
        freeTypeGroup->x() <= renderingGroup->x() ||
        freeTypeGroup->y() != renderingGroup->y()) return 9;
    // Configuration groups should remain compact even when the window is tall.
    if (renderingGroup->height() > renderingGroup->sizeHint().height() + 2 ||
        freeTypeGroup->height() > freeTypeGroup->sizeHint().height() + 2) return 12;

    window.setSplitPreview(true);
    if (splitter->count() != 2) return 2;
    auto *a = window.findChild<PreviewPane *>(QStringLiteral("paneA"));
    auto *b = window.findChild<PreviewPane *>(QStringLiteral("paneB"));
    if (!a || !b || a->fontFamily() != b->fontFamily() || !b->isActive()) return 3;
    app.processEvents();
    auto *renderingB = b->findChild<QGroupBox *>(QStringLiteral("renderingGroup"));
    auto *freeTypeB = b->findChild<QGroupBox *>(QStringLiteral("freetypeGroup"));
    if (!renderingB || !freeTypeB || !freeTypeB->isVisible() ||
        freeTypeB->x() <= renderingB->x() ||
        freeTypeB->y() != renderingB->y()) return 10;

    auto *rendererA = a->findChild<QComboBox *>(QStringLiteral("rendererCombo"));
    auto *rendererB = b->findChild<QComboBox *>(QStringLiteral("rendererCombo"));
    if (!rendererA || !rendererB || rendererA->currentText() != QStringLiteral("FreeType"))
        return 4;
    rendererB->setCurrentIndex(0); // Changing B must not change A.
    app.processEvents();
    if (!freeTypeB->isHidden() || !previewsAligned(a, b, &window)) return 11;
    if (rendererA->currentText() != QStringLiteral("FreeType") ||
        rendererB->currentText() != QStringLiteral("Qt")) return 5;

    // Alignment must hold with the backends reversed and with both using Qt.
    rendererA->setCurrentIndex(0);
    rendererB->setCurrentIndex(1);
    app.processEvents();
    if (!previewsAligned(a, b, &window)) return 13;
    rendererB->setCurrentIndex(0);
    app.processEvents();
    if (!previewsAligned(a, b, &window)) return 14;

    auto *sizeA = a->findChild<QSpinBox *>(QStringLiteral("sizeSpin"));
    auto *sizeB = b->findChild<QSpinBox *>(QStringLiteral("sizeSpin"));
    if (!sizeA || !sizeB) return 6;
    sizeB->setValue(sizeA->value() + 12);
    if (sizeA->value() == sizeB->value()) return 7;

    // A click on preview or a regular setting selects its pane; no explicit
    // select button or sacrificed mouse interaction is necessary.
    auto *previewA = a->findChild<QWidget *>(QStringLiteral("previewSurface"));
    auto *previewB = b->findChild<QWidget *>(QStringLiteral("previewSurface"));
    auto *boldB = b->findChild<QCheckBox *>();
    if (!previewA || !previewB || !boldB) return 15;
    click(previewA);
    if (!a->isActive() || b->isActive()) return 16;
    click(boldB);
    if (!b->isActive() || a->isActive()) return 17;
    click(previewA);
    if (!a->isActive() || b->isActive()) return 18;
    click(sizeB);
    if (!b->isActive() || a->isActive()) return 19;

    // The font browser must follow the pane last clicked, not whichever pane
    // was most recently created or which setting changed programmatically.
    auto *fontList = window.findChild<QListWidget *>(QStringLiteral("fontList"));
    if (fontList && fontList->count() >= 2) {
        click(previewA);
        const QString originalB = b->fontFamily();
        auto *replacement = fontList->item(0);
        if (replacement->text() == a->fontFamily()) replacement = fontList->item(1);
        fontList->setCurrentItem(replacement);
        if (a->fontFamily() != replacement->text() || b->fontFamily() != originalB)
            return 20;
    }
    app.processEvents(); // Exercise painting with both backends side by side.

    window.setSplitPreview(false);
    if (splitter->count() != 1 || !a->isActive()) return 8;

    const QStringList fontPaths = comparisonFontFiles();
    if (fontPaths.size() == 2) {
        MainWindow compareWindow;
        compareWindow.setPrimaryBackend(RenderBackend::FreeType);
        // A two-file load must create a split and target each pane explicitly,
        // even though creating a split normally makes B the active pane.
        QString error;
        if (!compareWindow.loadComparisonFonts(fontPaths.at(0), fontPaths.at(1), &error))
            return 22;
        auto *split = compareWindow.findChild<QSplitter *>(QStringLiteral("previewSplitter"));
        auto *left = compareWindow.findChild<PreviewPane *>(QStringLiteral("paneA"));
        auto *right = compareWindow.findChild<PreviewPane *>(QStringLiteral("paneB"));
        if (!split || split->count() != 2 || !left || !right ||
            left->fontFile() != QFileInfo(fontPaths.at(0)).absoluteFilePath() ||
            right->fontFile() != QFileInfo(fontPaths.at(1)).absoluteFilePath() ||
            left->fontFamily().isEmpty() || right->fontFamily().isEmpty() ||
            !left->isActive() || right->isActive())
            return 23;
        auto *leftRenderer = left->findChild<QComboBox *>(QStringLiteral("rendererCombo"));
        auto *rightRenderer = right->findChild<QComboBox *>(QStringLiteral("rendererCombo"));
        if (!leftRenderer || !rightRenderer ||
            leftRenderer->currentText() != QStringLiteral("FreeType") ||
            rightRenderer->currentText() != QStringLiteral("FreeType"))
            return 24;
        compareWindow.show();
        app.processEvents();
        if (!previewsAligned(left, right, &compareWindow)) return 25;

        // Re-loading the two files in reverse order must preserve both paths,
        // rather than accidentally routing both through the active pane.
        if (!compareWindow.loadComparisonFonts(fontPaths.at(1), fontPaths.at(0), &error) ||
            left->fontFile() != QFileInfo(fontPaths.at(1)).absoluteFilePath() ||
            right->fontFile() != QFileInfo(fontPaths.at(0)).absoluteFilePath())
            return 26;
        // A normal one-file open still targets the pane selected by a click.
        click(right->findChild<QWidget *>(QStringLiteral("previewSurface")));
        if (!compareWindow.loadFontFile(fontPaths.at(1), &error) ||
            left->fontFile() != QFileInfo(fontPaths.at(1)).absoluteFilePath() ||
            right->fontFile() != QFileInfo(fontPaths.at(1)).absoluteFilePath())
            return 27;

        // Regular/bold or differently built files may share a family. Both
        // panes must keep their own paths even in that case.
        const QStringList sameFamilyPaths = comparisonFontFiles(true);
        if (sameFamilyPaths.size() == 2) {
            if (!compareWindow.loadComparisonFonts(sameFamilyPaths.at(0),
                                                   sameFamilyPaths.at(1), &error) ||
                left->fontFile() != QFileInfo(sameFamilyPaths.at(0)).absoluteFilePath() ||
                right->fontFile() != QFileInfo(sameFamilyPaths.at(1)).absoluteFilePath())
                return 28;
        }
        const QString missing = QStringLiteral("__fontschau_missing_comparison_font__.ttf");
        if (compareWindow.loadComparisonFonts(fontPaths.at(0), missing, &error) ||
            !error.contains(missing))
            return 29;
    }
    return 0;
}
