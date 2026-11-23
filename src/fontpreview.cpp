// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fontpreview.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>

namespace {
constexpr int sizes[] = {12, 18, 24, 36, 48, 72};
const QString pangram = QStringLiteral("The quick brown fox jumps over the lazy dog");

QFont labelFont()
{
    QFont font = QApplication::font();
    font.setPointSize(10);
    return font;
}
}

FontPreview::FontPreview(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("fontPreview"));
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    m_sampleText = QStringLiteral("The quick brown fox jumps over the lazy dog.");
}

void FontPreview::setPreviewFont(const QFont &font)
{
    m_font = font;
    updateGeometry();
    update();
}

void FontPreview::setStyleName(const QString &style)
{
    m_style = style;
    update();
}

void FontPreview::setSampleText(const QString &text)
{
    m_sampleText = text.isEmpty()
        ? QStringLiteral("The quick brown fox jumps over the lazy dog.") : text;
    updateGeometry();
    update();
}

void FontPreview::setRendering(const RenderConfig &config)
{
    m_config = config;
    update();
}

QSize FontPreview::sizeHint() const
{
    QFontMetrics fm(m_font);
    int height = 40;
    int width = 320;
    for (const QString &line : m_sampleText.split(QLatin1Char('\n'))) {
        width = qMax(width, fm.horizontalAdvance(line) + 32);
        height += fm.height() + 4;
    }
    QFont current = m_font;
    for (int points : sizes) {
        current.setPointSize(points);
        QFontMetrics metrics(current);
        width = qMax(width, 75 + metrics.horizontalAdvance(pangram) + 32);
        height += metrics.height() + 8;
    }
    return QSize(width, height + 60);
}

QSize FontPreview::minimumSizeHint() const { return QSize(260, 200); }

void FontPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    if (m_config.backend == RenderBackend::FreeType)
        paintFreeType(painter);
    else
        paintQt(painter);
}

void FontPreview::paintQt(QPainter &painter)
{
    const QPalette pal = palette();
    constexpr int margin = 16;
    int y = margin;
    painter.setPen(pal.text().color());
    painter.setFont(m_font);
    QFontMetrics fm(m_font);
    for (const QString &line : m_sampleText.split(QLatin1Char('\n'))) {
        painter.drawText(margin, y + fm.ascent(), line);
        y += fm.height() + 4;
    }
    y += 12;
    painter.setPen(pal.mid().color());
    painter.drawLine(margin, y, width() - margin, y);
    y += 16;

    QFont label = labelFont();
    label.setBold(true);
    painter.setPen(pal.text().color());
    painter.setFont(label);
    painter.drawText(margin, y + QFontMetrics(label).ascent(), tr("Size samples"));
    y += QFontMetrics(label).height() + 10;
    label.setBold(false);
    const QFontMetrics labelMetrics(label);
    for (int size : sizes) {
        QFont sample = m_font;
        sample.setPointSize(size);
        QFontMetrics metrics(sample);
        const QString sizeLabel = QString::number(size) + QStringLiteral(" pt");
        painter.setFont(label);
        painter.drawText(margin, y + labelMetrics.ascent(), sizeLabel);
        painter.setFont(sample);
        painter.drawText(margin + labelMetrics.horizontalAdvance(sizeLabel) + 12,
                         y + metrics.ascent(), pangram);
        y += qMax(metrics.height(), labelMetrics.height()) + 8;
    }
}

void FontPreview::paintFreeType(QPainter &painter)
{
    QString error;
    if (!m_freetype.configure(m_font, m_style, m_config, &error) ||
        !m_freetype.setPointSize(qMax(1, m_font.pointSize()), logicalDpiY())) {
        painter.setPen(palette().text().color());
        painter.drawText(rect().adjusted(16, 16, -16, -16),
                         Qt::AlignTop | Qt::TextWordWrap,
                         error.isEmpty() ? tr("Unable to set FreeType font size") : error);
        return;
    }

    // Each channel of an LCD glyph has its own coverage. QPainter's single
    // alpha channel cannot represent this, so composite into an opaque RGB
    // image instead, using the window's palette and the selected gamma.
    QImage image(size(), QImage::Format_RGB32);
    image.fill(palette().base().color());
    const QColor ink = palette().text().color();
    constexpr int margin = 16;
    int y = margin;
    auto metrics = m_freetype.metrics();
    for (const QString &line : m_sampleText.split(QLatin1Char('\n'))) {
        m_freetype.drawText(image, margin, y + metrics.ascent, line, ink);
        y += metrics.height + 4;
    }
    y += 12;
    {
        QPainter labels(&image);
        labels.setPen(palette().mid().color());
        labels.drawLine(margin, y, width() - margin, y);
        y += 16;
        QFont label = labelFont();
        label.setBold(true);
        labels.setFont(label);
        labels.setPen(ink);
        labels.drawText(margin, y + QFontMetrics(label).ascent(), tr("Size samples"));
    }
    y += QFontMetrics(labelFont()).height() + 10;
    const QFontMetrics labelMetrics(labelFont());
    for (int size : sizes) {
        if (!m_freetype.setPointSize(size, logicalDpiY()))
            continue;
        metrics = m_freetype.metrics();
        const QString sizeLabel = QString::number(size) + QStringLiteral(" pt");
        {
            QPainter labels(&image);
            labels.setPen(ink);
            labels.setFont(labelFont());
            labels.drawText(margin, y + labelMetrics.ascent(), sizeLabel);
        }
        m_freetype.drawText(image, margin + labelMetrics.horizontalAdvance(sizeLabel) + 12,
                            y + metrics.ascent, pangram, ink);
        y += qMax(metrics.height, labelMetrics.height()) + 8;
    }
    painter.drawImage(0, 0, image);
}
