// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

enum class RenderBackend { Qt, FreeType };
enum class AntialiasMode { System, Gray, Mono, LcdRGB, LcdBGR, LcdVRGB, LcdVBGR };
enum class HintMode { Default, None, Light, Full };
enum class LcdFilter { None, Default, Light, Legacy };

struct RenderConfig
{
    RenderBackend backend = RenderBackend::Qt;
    AntialiasMode antialias = AntialiasMode::System;
    HintMode hinting = HintMode::Default;
    LcdFilter lcdFilter = LcdFilter::Default;
    bool stemDarkening = false;
    double embolden = 0.0; // Additional FreeType outline strength, in pixels
    double gamma = 1.0;    // Coverage transfer exponent before compositing
    QString fontFile;      // Override Fontconfig for loaded application fonts
};
