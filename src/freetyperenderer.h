// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "renderconfig.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QString>

#include <memory>

// Direct FreeType rasterization into a QImage. Fontconfig is used only to
// resolve installed font faces; fonts opened from disk bypass Fontconfig.
// This renderer does not use QPainter::drawText or Qt's font rasterizer.
class FreeTypeRenderer
{
public:
    struct Metrics {
        int ascent = 0;
        int descent = 0;
        int height = 0;
    };

    FreeTypeRenderer();
    ~FreeTypeRenderer();
    FreeTypeRenderer(const FreeTypeRenderer &) = delete;
    FreeTypeRenderer &operator=(const FreeTypeRenderer &) = delete;

    bool configure(const QFont &font, const QString &style,
                   const RenderConfig &config, QString *error);
    bool setPointSize(int points, int dpi);
    Metrics metrics() const;
    int textWidth(const QString &text) const;
    void drawText(QImage &target, int x, int baseline, const QString &text,
                  const QColor &color) const;

private:
    struct Data;
    std::unique_ptr<Data> m;
};
