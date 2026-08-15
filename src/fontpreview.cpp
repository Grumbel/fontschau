#include "fontpreview.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QApplication>
#include <QStyle>

FontPreview::FontPreview(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    m_sampleText = QStringLiteral("The quick brown fox jumps over the lazy dog.");
    rebuildAutoSamples();
}

void FontPreview::setPreviewFont(const QFont &font)
{
    m_font = font;
    m_baseSize = font.pointSize() > 0 ? font.pointSize() : 24;
    rebuildAutoSamples();
    updateGeometry();
    update();
}

void FontPreview::setSampleText(const QString &text)
{
    m_sampleText = text.isEmpty()
                       ? QStringLiteral("The quick brown fox jumps over the lazy dog.")
                       : text;
    updateGeometry();
    update();
}

void FontPreview::rebuildAutoSamples()
{
    // Generate a few size variants for automatic multi-size preview
    m_autoSamples.clear();
    const int sizes[] = {12, 18, 24, 36, 48, 72};
    for (int s : sizes) {
        m_autoSamples << QString::number(s) + QStringLiteral(" pt");
    }
}

QSize FontPreview::sizeHint() const
{
    QFontMetrics fm(m_font);
    const QStringList lines = m_sampleText.split(QLatin1Char('\n'));
    int width = 400;
    int height = 40;

    for (const QString &line : lines) {
        width = qMax(width, fm.horizontalAdvance(line) + 40);
        height += fm.height() + 4;
    }

    // Extra space for multi-size section
    height += 200;
    return QSize(width, height);
}

QSize FontPreview::minimumSizeHint() const
{
    return QSize(300, 200);
}

void FontPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPalette pal = palette();
    painter.fillRect(rect(), pal.base());

    int y = 16;
    const int margin = 16;

    // --- Custom / user sample text at current size ---
    {
        QFont f = m_font;
        painter.setFont(f);
        QFontMetrics fm(f);
        painter.setPen(pal.text().color());

        const QStringList lines = m_sampleText.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            painter.drawText(margin, y + fm.ascent(), line);
            y += fm.height() + 4;
        }
        y += 12;
    }

    // Separator
    painter.setPen(pal.mid().color());
    painter.drawLine(margin, y, width() - margin, y);
    y += 16;

    // --- Automatic multi-size samples ---
    painter.setPen(pal.text().color());
    QFont labelFont = QApplication::font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.drawText(margin, y + QFontMetrics(labelFont).ascent(),
                     tr("Size samples"));
    y += QFontMetrics(labelFont).height() + 10;

    const int sizes[] = {12, 18, 24, 36, 48, 72};
    const QString pangram = QStringLiteral("The quick brown fox jumps over the lazy dog");

    for (int s : sizes) {
        QFont f = m_font;
        f.setPointSize(s);
        painter.setFont(f);
        QFontMetrics fm(f);

        // Size label in a smaller consistent font
        QFont sizeLabel = labelFont;
        sizeLabel.setBold(false);
        painter.setFont(sizeLabel);
        const QString label = QString::number(s) + QStringLiteral(" pt");
        painter.drawText(margin, y + QFontMetrics(sizeLabel).ascent(), label);

        const int labelWidth = QFontMetrics(sizeLabel).horizontalAdvance(label) + 12;
        painter.setFont(f);
        painter.drawText(margin + labelWidth, y + fm.ascent(), pangram);

        y += qMax(fm.height(), QFontMetrics(sizeLabel).height()) + 8;
        if (y > height() - 20)
            break;
    }
}