// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fontpreview.h"

#include <QWidget>
#include <QString>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;

// One independently configured comparison pane. The source text is shared by
// the window, but font family, face, size and rendering settings are per-pane.
class PreviewPane : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewPane(const QString &title, bool closable, QWidget *parent = nullptr);
    QString fontFamily() const { return m_family; }
    QString fontFile() const { return m_file; }
    void setFontFamily(const QString &family, const QString &file = {});
    void setSampleText(const QString &text);
    void setActive(bool active);
    void setBackend(RenderBackend backend);
    void copyFrom(const PreviewPane &other);

signals:
    void activated(PreviewPane *pane);
    void closeRequested(PreviewPane *pane);

private:
    void updatePreview();
    void updateBackend(int index);
    void rebuildAntialiasOptions();
    RenderConfig renderConfig() const;

    QString m_title;
    QString m_family;
    QString m_file;
    QString m_style;
    bool m_updating = false;
    AntialiasMode m_qtAntialias = AntialiasMode::System;
    AntialiasMode m_ftAntialias = AntialiasMode::Gray;

    QPushButton *m_activate = nullptr;
    QComboBox *m_styleCombo = nullptr;
    QSpinBox *m_size = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QCheckBox *m_bold = nullptr;
    QCheckBox *m_italic = nullptr;
    QCheckBox *m_underline = nullptr;
    QCheckBox *m_strike = nullptr;
    QComboBox *m_backend = nullptr;
    QComboBox *m_antialias = nullptr;
    QComboBox *m_hinting = nullptr;
    QGroupBox *m_advanced = nullptr;
    QComboBox *m_filter = nullptr;
    QCheckBox *m_darkening = nullptr;
    QDoubleSpinBox *m_embolden = nullptr;
    QDoubleSpinBox *m_gamma = nullptr;
    FontPreview *m_preview = nullptr;
    QLabel *m_info = nullptr;
};
