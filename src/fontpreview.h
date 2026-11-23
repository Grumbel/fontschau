// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "freetyperenderer.h"
#include "renderconfig.h"

#include <QFont>
#include <QString>
#include <QWidget>

class QPainter;

class FontPreview : public QWidget
{
    Q_OBJECT
public:
    explicit FontPreview(QWidget *parent = nullptr);
    void setPreviewFont(const QFont &font);
    void setStyleName(const QString &style);
    void setSampleText(const QString &text);
    void setRendering(const RenderConfig &config);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont m_font;
    QString m_style;
    QString m_sampleText;
    RenderConfig m_config;
    FreeTypeRenderer m_freetype;
    void paintQt(QPainter &painter);
    void paintFreeType(QPainter &painter);
};
