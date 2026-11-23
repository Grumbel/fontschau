// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QListWidget>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QSpinBox>

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
    return 0;
}
