// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "freetyperenderer.h"

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H
#include FT_OUTLINE_H
#include FT_SYNTHESIS_H

#include <QtGlobal>
#include <QFile>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
QString familyWithoutFoundry(QString family)
{
    // Qt may expose family names as "Family [Foundry]"; Fontconfig expects
    // the underlying family name without this Qt presentation suffix.
    const int start = family.lastIndexOf(QStringLiteral(" ["));
    if (start >= 0 && family.endsWith(QLatin1Char(']')))
        family.truncate(start);
    return family;
}

QString resolveFont(const QFont &font, const QString &style, int *index)
{
    *index = 0;
    FcPattern *pattern = FcPatternCreate();
    if (!pattern)
        return {};
    const QByteArray family = familyWithoutFoundry(font.family()).toUtf8();
    const QByteArray faceStyle = style.toUtf8();
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8 *>(family.constData()));
    if (!style.isEmpty())
        FcPatternAddString(pattern, FC_STYLE, reinterpret_cast<const FcChar8 *>(faceStyle.constData()));
    FcPatternAddInteger(pattern, FC_WEIGHT, font.bold() ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, font.italic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result = FcResultNoMatch;
    FcPattern *match = FcFontMatch(nullptr, pattern, &result);
    FcPatternDestroy(pattern);
    if (!match)
        return {};

    FcChar8 *file = nullptr;
    QString path;
    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch)
        path = QString::fromUtf8(reinterpret_cast<const char *>(file));
    FcPatternGetInteger(match, FC_INDEX, 0, index);
    FcPatternDestroy(match);
    return path;
}

FT_Render_Mode renderMode(AntialiasMode mode)
{
    switch (mode) {
    case AntialiasMode::Mono: return FT_RENDER_MODE_MONO;
    case AntialiasMode::LcdRGB:
    case AntialiasMode::LcdBGR: return FT_RENDER_MODE_LCD;
    case AntialiasMode::LcdVRGB:
    case AntialiasMode::LcdVBGR: return FT_RENDER_MODE_LCD_V;
    default: return FT_RENDER_MODE_NORMAL;
    }
}

FT_Int32 loadFlags(const RenderConfig &cfg)
{
    FT_Int32 flags = FT_LOAD_DEFAULT;
    if (cfg.hinting == HintMode::None)
        flags |= FT_LOAD_NO_HINTING;
    else if (cfg.hinting == HintMode::Light)
        flags |= FT_LOAD_TARGET_LIGHT;
    else if (cfg.hinting == HintMode::Full)
        flags |= FT_LOAD_TARGET_NORMAL;
    else {
        switch (cfg.antialias) {
        case AntialiasMode::Mono: flags |= FT_LOAD_TARGET_MONO; break;
        case AntialiasMode::LcdRGB:
        case AntialiasMode::LcdBGR: flags |= FT_LOAD_TARGET_LCD; break;
        case AntialiasMode::LcdVRGB:
        case AntialiasMode::LcdVBGR: flags |= FT_LOAD_TARGET_LCD_V; break;
        default: break;
        }
    }
    return flags;
}

int coverage(int value, double gamma)
{
    if (value <= 0) return 0;
    if (value >= 255) return 255;
    return qBound(0, int(std::lround(std::pow(value / 255.0, gamma) * 255.0)), 255);
}

int mix(int old, int ink, int a)
{
    return (old * (255 - a) + ink * a + 127) / 255;
}

const unsigned char *bitmapRow(const FT_Bitmap &bitmap, int row)
{
    const int pitch = bitmap.pitch;
    return bitmap.buffer + (pitch >= 0 ? row * pitch : (int(bitmap.rows) - 1 - row) * (-pitch));
}

void blendBitmap(QImage &target, const FT_Bitmap &bitmap, int left, int top,
                 const QColor &ink, const RenderConfig &cfg)
{
    if (!bitmap.buffer || target.format() != QImage::Format_RGB32)
        return;
    const bool horizontal = bitmap.pixel_mode == FT_PIXEL_MODE_LCD;
    const bool vertical = bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V;
    const int cols = horizontal ? int(bitmap.width) / 3 : int(bitmap.width);
    const int rows = vertical ? int(bitmap.rows) / 3 : int(bitmap.rows);
    for (int y = 0; y < rows; ++y) {
        if (top + y < 0 || top + y >= target.height()) continue;
        QRgb *dst = reinterpret_cast<QRgb *>(target.scanLine(top + y));
        for (int x = 0; x < cols; ++x) {
            if (left + x < 0 || left + x >= target.width()) continue;
            int r = 0, g = 0, b = 0;
            if (horizontal || vertical) {
                if (horizontal) {
                    const unsigned char *p = bitmapRow(bitmap, y) + x * 3;
                    r = p[0]; g = p[1]; b = p[2];
                } else {
                    r = bitmapRow(bitmap, y * 3)[x];
                    g = bitmapRow(bitmap, y * 3 + 1)[x];
                    b = bitmapRow(bitmap, y * 3 + 2)[x];
                }
                if (cfg.antialias == AntialiasMode::LcdBGR ||
                    cfg.antialias == AntialiasMode::LcdVBGR)
                    std::swap(r, b);
            } else if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                r = g = b = bitmapRow(bitmap, y)[x];
            } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                r = g = b = (bitmapRow(bitmap, y)[x / 8] & (0x80 >> (x % 8))) ? 255 : 0;
            } else {
                continue;
            }
            r = coverage(r, cfg.gamma);
            g = coverage(g, cfg.gamma);
            b = coverage(b, cfg.gamma);
            const QRgb prev = dst[left + x];
            dst[left + x] = qRgb(mix(qRed(prev), ink.red(), r),
                                 mix(qGreen(prev), ink.green(), g),
                                 mix(qBlue(prev), ink.blue(), b));
        }
    }
}
} // namespace

struct FreeTypeRenderer::Data
{
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    QString filename;
    int index = -1;
    QString requestedFamily;
    QString requestedStyle;
    int requestedWeight = 0;
    bool requestedItalic = false;
    QFont font;
    RenderConfig config;
    FT_Int32 flags = 0;

    ~Data()
    {
        if (face) FT_Done_Face(face);
        if (library) FT_Done_FreeType(library);
    }

    // Lookup face in a TTC/OTC file by family and style where possible.
    int faceIndex(const QString &file, const QFont &font, const QString &style)
    {
        FT_Face probe = nullptr;
        if (FT_New_Face(library, QFile::encodeName(file).constData(), -1, &probe))
            return 0;
        const long faces = probe->num_faces;
        FT_Done_Face(probe);
        int familyMatch = 0;
        for (long i = 0; i < faces; ++i) {
            FT_Face candidate = nullptr;
            if (FT_New_Face(library, QFile::encodeName(file).constData(), i, &candidate))
                continue;
            const bool familyOk = QString::fromUtf8(candidate->family_name).compare(
                familyWithoutFoundry(font.family()), Qt::CaseInsensitive) == 0;
            const bool styleOk = QString::fromUtf8(candidate->style_name).compare(style, Qt::CaseInsensitive) == 0;
            FT_Done_Face(candidate);
            if (familyOk && styleOk)
                return int(i);
            if (familyOk)
                familyMatch = int(i);
        }
        return familyMatch;
    }
};

FreeTypeRenderer::FreeTypeRenderer() : m(std::make_unique<Data>()) {}
FreeTypeRenderer::~FreeTypeRenderer() = default;

bool FreeTypeRenderer::configure(const QFont &font, const QString &style,
                                 const RenderConfig &config, QString *error)
{
    if (!m->library && FT_Init_FreeType(&m->library)) {
        if (error) *error = QStringLiteral("FreeType initialization failed");
        return false;
    }
    if (config.fontFile.isEmpty() && !FcInit()) {
        if (error) *error = QStringLiteral("Fontconfig initialization failed");
        return false;
    }
    int index = m->index;
    const bool sameRequest = m->face &&
        m->requestedFamily == font.family() && m->requestedStyle == style &&
        m->requestedWeight == font.weight() &&
        m->requestedItalic == font.italic();
    const QString filename = config.fontFile.isEmpty()
        ? (sameRequest && m->config.fontFile.isEmpty()
           ? m->filename : resolveFont(font, style, &index))
        : config.fontFile;
    if (filename.isEmpty()) {
        if (error) *error = QStringLiteral("Fontconfig could not resolve %1").arg(font.family());
        return false;
    }
    if (!config.fontFile.isEmpty() &&
        (!sameRequest || filename != m->filename || !m->face))
        index = m->faceIndex(filename, font, style);

    if (filename != m->filename || index != m->index || !m->face) {
        if (m->face) FT_Done_Face(m->face);
        m->face = nullptr;
        if (FT_New_Face(m->library, QFile::encodeName(filename).constData(), index, &m->face)) {
            m->filename.clear();
            m->index = -1;
            if (error) *error = QStringLiteral("FreeType could not open %1").arg(filename);
            return false;
        }
        m->filename = filename;
        m->index = index;
    }
    m->font = font;
    m->requestedFamily = font.family();
    m->requestedStyle = style;
    m->requestedWeight = font.weight();
    m->requestedItalic = font.italic();
    m->config = config;
    m->flags = loadFlags(config);

    // These options live on this private FreeType library, not the global Qt
    // library. Drivers that do not implement the property simply ignore it.
    FT_Bool noDarkening = config.stemDarkening ? 0 : 1;
    for (const char *module : {"cff", "autofitter", "type1", "t1cid"})
        FT_Property_Set(m->library, module, "no-stem-darkening", &noDarkening);

    FT_LcdFilter filter = FT_LCD_FILTER_DEFAULT;
    switch (config.lcdFilter) {
    case LcdFilter::None: filter = FT_LCD_FILTER_NONE; break;
    case LcdFilter::Default: filter = FT_LCD_FILTER_DEFAULT; break;
    case LcdFilter::Light: filter = FT_LCD_FILTER_LIGHT; break;
    case LcdFilter::Legacy: filter = FT_LCD_FILTER_LEGACY; break;
    }
    // Some FreeType builds lack LCD filter support; rasterization still works.
    FT_Library_SetLcdFilter(m->library, filter);
    return true;
}

bool FreeTypeRenderer::setPointSize(int points, int dpi)
{
    return m->face && !FT_Set_Char_Size(m->face, 0, FT_F26Dot6(points * 64),
                                       FT_UInt(dpi), FT_UInt(dpi));
}

FreeTypeRenderer::Metrics FreeTypeRenderer::metrics() const
{
    if (!m->face || !m->face->size) return {};
    const auto &ft = m->face->size->metrics;
    const int ascent = int((ft.ascender + 63) >> 6);
    const int descent = int((-ft.descender + 63) >> 6);
    const int height = qMax(ascent + descent, int((ft.height + 63) >> 6));
    return {ascent, descent, height};
}

int FreeTypeRenderer::textWidth(const QString &text) const
{
    if (!m->face) return 0;
    FT_UInt previous = 0;
    int width = 0;
    for (uint character : text.toUcs4()) {
        const FT_UInt glyph = FT_Get_Char_Index(m->face, character);
        if (previous && glyph && FT_HAS_KERNING(m->face)) {
            FT_Vector delta{};
            FT_Get_Kerning(m->face, previous, glyph, FT_KERNING_DEFAULT, &delta);
            width += int(delta.x >> 6);
        }
        if (!FT_Load_Glyph(m->face, glyph, m->flags))
            width += int(m->face->glyph->advance.x >> 6);
        previous = glyph;
    }
    return width;
}

void FreeTypeRenderer::drawText(QImage &target, int x, int baseline,
                               const QString &text, const QColor &ink) const
{
    if (!m->face) return;
    const FT_Render_Mode mode = renderMode(m->config.antialias);
    FT_UInt previous = 0;
    const int lineStart = x;
    for (uint character : text.toUcs4()) {
        const FT_UInt glyph = FT_Get_Char_Index(m->face, character);
        if (previous && glyph && FT_HAS_KERNING(m->face)) {
            FT_Vector delta{};
            FT_Get_Kerning(m->face, previous, glyph, FT_KERNING_DEFAULT, &delta);
            x += int(delta.x >> 6);
        }
        if (FT_Load_Glyph(m->face, glyph, m->flags)) {
            previous = glyph;
            continue;
        }
        FT_GlyphSlot slot = m->face->glyph;
        // Synthetic style and adjustable outline emboldening before rasterizing.
        if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
            if (m->font.italic() && !(m->face->style_flags & FT_STYLE_FLAG_ITALIC))
                FT_GlyphSlot_Oblique(slot);
            if (m->font.bold() && !(m->face->style_flags & FT_STYLE_FLAG_BOLD))
                FT_GlyphSlot_Embolden(slot);
            if (m->config.embolden > 0)
                FT_Outline_Embolden(&slot->outline, FT_Pos(std::lround(m->config.embolden * 64)));
        }
        const int advance = int(slot->advance.x >> 6);
        FT_Error renderError = FT_Render_Glyph(slot, mode);
        // Graceful fallback for FreeType builds lacking LCD raster support.
        if (renderError && (mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V))
            renderError = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (!renderError)
            blendBitmap(target, slot->bitmap, x + slot->bitmap_left,
                        baseline - slot->bitmap_top, ink, m->config);
        x += advance;
        previous = glyph;
    }
    // Decorations are not encoded in a FreeType face's glyph bitmap.
    // Draw them explicitly so Qt and FreeType honor the same style toggles.
    if (m->font.underline() || m->font.strikeOut()) {
        const Metrics size = metrics();
        const int thickness = qMax(1, size.height / 20);
        const auto drawRule = [&](int y) {
            for (int row = y; row < y + thickness; ++row) {
                if (row < 0 || row >= target.height()) continue;
                QRgb *pixels = reinterpret_cast<QRgb *>(target.scanLine(row));
                for (int col = qMax(0, lineStart); col < qMin(target.width(), x); ++col)
                    pixels[col] = ink.rgb();
            }
        };
        if (m->font.underline()) drawRule(baseline + qMax(1, size.descent / 3));
        if (m->font.strikeOut()) drawRule(baseline - size.ascent / 3);
    }
}
