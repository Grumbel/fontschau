// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "freetyperenderer.h"

#include <QGuiApplication>
#include <QFont>
#include <QImage>
#include <QString>

#include <cstdio>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    FreeTypeRenderer renderer;
    QFont font(QStringLiteral("sans-serif"));
    RenderConfig config;
    config.backend = RenderBackend::FreeType;
    QString error;
    if (!renderer.configure(font, QString(), config, &error) ||
        !renderer.setPointSize(32, 96)) {
        std::fprintf(stderr, "FreeType configuration failed: %s\n", qPrintable(error));
        return 1;
    }
    QImage plain(400, 100, QImage::Format_RGB32);
    plain.fill(Qt::white);
    renderer.drawText(plain, 10, 55, QStringLiteral("Fontschau test"), Qt::black);
    bool hasInk = false;
    for (int y = 0; y < plain.height() && !hasInk; ++y)
        for (int x = 0; x < plain.width(); ++x)
            if (plain.pixel(x, y) != qRgb(255, 255, 255)) { hasInk = true; break; }
    if (!hasInk) return 2;

    for (AntialiasMode mode : {AntialiasMode::Mono, AntialiasMode::Gray,
                               AntialiasMode::LcdRGB, AntialiasMode::LcdBGR,
                               AntialiasMode::LcdVRGB, AntialiasMode::LcdVBGR}) {
        config.antialias = mode;
        config.lcdFilter = LcdFilter::Light;
        config.gamma = 1.5;
        config.embolden = 0.5;
        if (!renderer.configure(font, {}, config, &error) || !renderer.setPointSize(24, 96))
            return 3;
        QImage image(300, 80, QImage::Format_RGB32);
        image.fill(Qt::white);
        renderer.drawText(image, 12, 48, QStringLiteral("RGB Test"), Qt::black);
        bool drawn = false;
        for (int y = 0; y < image.height() && !drawn; ++y)
            for (int x = 0; x < image.width(); ++x)
                if (image.pixel(x, y) != qRgb(255, 255, 255)) { drawn = true; break; }
        if (!drawn) return 4;
    }
    return 0;
}
