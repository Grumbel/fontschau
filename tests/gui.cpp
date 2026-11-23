// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QSplitter>
#include <QSpinBox>

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
    window.setSplitPreview(true);
    if (splitter->count() != 2) return 2;
    auto *a = window.findChild<PreviewPane *>(QStringLiteral("paneA"));
    auto *b = window.findChild<PreviewPane *>(QStringLiteral("paneB"));
    if (!a || !b || a->fontFamily() != b->fontFamily()) return 3;
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
    if (!freeTypeB->isHidden()) return 11;
    if (rendererA->currentText() != QStringLiteral("FreeType") ||
        rendererB->currentText() != QStringLiteral("Qt")) return 5;
    auto *sizeA = a->findChild<QSpinBox *>(QStringLiteral("sizeSpin"));
    auto *sizeB = b->findChild<QSpinBox *>(QStringLiteral("sizeSpin"));
    if (!sizeA || !sizeB) return 6;
    sizeB->setValue(sizeA->value() + 12);
    if (sizeA->value() == sizeB->value()) return 7;
    app.processEvents(); // Exercise painting with both backends side by side.
    window.setSplitPreview(false);
    if (splitter->count() != 1) return 8;
    return 0;
}
