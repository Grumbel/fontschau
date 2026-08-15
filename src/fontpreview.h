// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FONTPREVIEW_H
#define FONTPREVIEW_H

#include <QWidget>
#include <QFont>
#include <QString>
#include <QStringList>

class FontPreview : public QWidget
{
    Q_OBJECT

public:
    explicit FontPreview(QWidget *parent = nullptr);

    void setPreviewFont(const QFont &font);
    void setSampleText(const QString &text);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont m_font;
    QString m_sampleText;
    QStringList m_autoSamples;
    int m_baseSize = 24;

    void rebuildAutoSamples();
};

#endif // FONTPREVIEW_H