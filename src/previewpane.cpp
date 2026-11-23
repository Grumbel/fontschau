// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "previewpane.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFontInfo>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
template <typename T> void addChoice(QComboBox *combo, const QString &name, T value)
{
    combo->addItem(name, static_cast<int>(value));
}

void selectChoice(QComboBox *combo, int value)
{
    const int i = combo->findData(value);
    if (i >= 0) combo->setCurrentIndex(i);
}
}

PreviewPane::PreviewPane(const QString &title, bool closable, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(closable ? QStringLiteral("paneB") : QStringLiteral("paneA"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(5);

    // Keep controls compact. In comparison mode both panes reserve the same
    // control-area height so their scrollable previews share an exact top edge.
    m_controls = new QWidget(this);
    m_controls->setObjectName(QStringLiteral("paneControls"));
    m_controls->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *controlsLayout = new QVBoxLayout(m_controls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(5);
    layout->addWidget(m_controls);

    auto *header = new QHBoxLayout;
    m_heading = new QLabel(title);
    m_heading->setObjectName(QStringLiteral("paneHeading"));
    m_heading->setToolTip(tr("Click the preview or settings to select this pane for the font browser"));
    header->addWidget(m_heading);
    header->addStretch();
    if (closable) {
        auto *close = new QPushButton(tr("Close pane"));
        connect(close, &QPushButton::clicked, this, [this] { emit closeRequested(this); });
        header->addWidget(close);
    }
    controlsLayout->addLayout(header);

    auto *properties = new QGroupBox(tr("Font"));
    properties->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *fontGrid = new QGridLayout(properties);
    fontGrid->addWidget(new QLabel(tr("Style:")), 0, 0);
    m_styleCombo = new QComboBox;
    m_styleCombo->setObjectName(QStringLiteral("styleCombo"));
    fontGrid->addWidget(m_styleCombo, 0, 1);
    fontGrid->addWidget(new QLabel(tr("Size:")), 0, 2);
    m_size = new QSpinBox;
    m_size->setObjectName(QStringLiteral("sizeSpin"));
    m_size->setRange(6, 200);
    m_size->setValue(24);
    m_size->setSuffix(tr(" pt"));
    fontGrid->addWidget(m_size, 0, 3);
    m_sizeSlider = new QSlider(Qt::Horizontal);
    m_sizeSlider->setRange(6, 120);
    m_sizeSlider->setValue(24);
    fontGrid->addWidget(m_sizeSlider, 1, 0, 1, 4);
    connect(m_sizeSlider, &QSlider::valueChanged, m_size, &QSpinBox::setValue);
    connect(m_size, QOverload<int>::of(&QSpinBox::valueChanged), m_sizeSlider, &QSlider::setValue);
    auto *styles = new QHBoxLayout;
    m_bold = new QCheckBox(tr("Bold"));
    m_italic = new QCheckBox(tr("Italic"));
    m_underline = new QCheckBox(tr("Underline"));
    m_strike = new QCheckBox(tr("Strikeout"));
    for (QCheckBox *check : {m_bold, m_italic, m_underline, m_strike})
        styles->addWidget(check);
    fontGrid->addLayout(styles, 2, 0, 1, 4);
    controlsLayout->addWidget(properties);

    auto *render = new QGroupBox(tr("Rendering"));
    render->setObjectName(QStringLiteral("renderingGroup"));
    render->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *renderGrid = new QGridLayout(render);
    renderGrid->addWidget(new QLabel(tr("Engine:")), 0, 0);
    m_backend = new QComboBox;
    m_backend->setObjectName(QStringLiteral("rendererCombo"));
    addChoice(m_backend, tr("Qt"), RenderBackend::Qt);
    addChoice(m_backend, tr("FreeType"), RenderBackend::FreeType);
    renderGrid->addWidget(m_backend, 0, 1);
    renderGrid->addWidget(new QLabel(tr("Antialias:")), 1, 0);
    m_antialias = new QComboBox;
    m_antialias->setObjectName(QStringLiteral("antialiasCombo"));
    // Long option names should not force a narrow comparison pane wider.
    m_antialias->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_antialias->setMinimumContentsLength(11);
    renderGrid->addWidget(m_antialias, 1, 1);
    renderGrid->addWidget(new QLabel(tr("Hinting:")), 2, 0);
    m_hinting = new QComboBox;
    addChoice(m_hinting, tr("Default"), HintMode::Default);
    addChoice(m_hinting, tr("None"), HintMode::None);
    addChoice(m_hinting, tr("Light"), HintMode::Light);
    addChoice(m_hinting, tr("Full"), HintMode::Full);
    renderGrid->addWidget(m_hinting, 2, 1);
    m_advanced = new QGroupBox(tr("FreeType options"));
    m_advanced->setObjectName(QStringLiteral("freetypeGroup"));
    m_advanced->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *advancedGrid = new QGridLayout(m_advanced);
    advancedGrid->addWidget(new QLabel(tr("LCD filter:")), 0, 0);
    m_filter = new QComboBox;
    addChoice(m_filter, tr("None"), LcdFilter::None);
    addChoice(m_filter, tr("Default"), LcdFilter::Default);
    addChoice(m_filter, tr("Light"), LcdFilter::Light);
    addChoice(m_filter, tr("Legacy"), LcdFilter::Legacy);
    m_filter->setCurrentIndex(1);
    advancedGrid->addWidget(m_filter, 0, 1);
    m_darkening = new QCheckBox(tr("Stem darkening"));
    m_darkening->setToolTip(tr("FreeType driver property; not supported by all fonts or drivers."));
    advancedGrid->addWidget(m_darkening, 1, 0, 1, 2);
    advancedGrid->addWidget(new QLabel(tr("Extra embolden:")), 2, 0);
    m_embolden = new QDoubleSpinBox;
    m_embolden->setRange(0.0, 3.0);
    m_embolden->setSingleStep(0.25);
    m_embolden->setSuffix(tr(" px"));
    m_embolden->setToolTip(tr("Synthetic outline thickening in addition to the font's own weight."));
    advancedGrid->addWidget(m_embolden, 2, 1);
    advancedGrid->addWidget(new QLabel(tr("Coverage gamma:")), 3, 0);
    m_gamma = new QDoubleSpinBox;
    m_gamma->setRange(0.4, 3.0);
    m_gamma->setSingleStep(0.1);
    m_gamma->setValue(1.0);
    m_gamma->setToolTip(tr("Exponent applied to glyph coverage during compositing (1.0 = linear coverage)."));
    advancedGrid->addWidget(m_gamma, 3, 1);
    // Keep the common rendering settings and FreeType-specific settings on
    // the same row, including when the preview is split into two panes.
    auto *renderingRow = new QHBoxLayout;
    renderingRow->addWidget(render, 0, Qt::AlignTop);
    renderingRow->addWidget(m_advanced, 0, Qt::AlignTop);
    renderingRow->addStretch(1); // Leave free width; do not stretch the groups.
    controlsLayout->addLayout(renderingRow);
    controlsLayout->addStretch(1); // Extra alignment space stays BELOW the settings.

    m_preview = new FontPreview;
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("previewScroll"));
    m_preview->setObjectName(QStringLiteral("previewSurface"));
    scroll->setWidget(m_preview);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(scroll, 1);
    m_info = new QLabel;
    m_info->setWordWrap(true);
    m_info->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_info);

    connect(m_styleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (m_updating) return;
        m_style = m_styleCombo->currentText();
        QFontDatabase db;
        QSignalBlocker b(m_bold), i(m_italic);
        m_bold->setChecked(db.bold(m_family, m_style));
        m_italic->setChecked(db.italic(m_family, m_style));
        updatePreview();
    });
    connect(m_size, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewPane::updatePreview);
    for (QCheckBox *check : {m_bold, m_italic, m_underline, m_strike})
        connect(check, &QCheckBox::toggled, this, &PreviewPane::updatePreview);
    connect(m_backend, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PreviewPane::updateBackend);
    connect(m_antialias, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (m_updating) return;
        const auto aa = static_cast<AntialiasMode>(m_antialias->currentData().toInt());
        if (static_cast<RenderBackend>(m_backend->currentData().toInt()) == RenderBackend::Qt)
            m_qtAntialias = aa;
        else
            m_ftAntialias = aa;
        const bool lcd = int(aa) >= int(AntialiasMode::LcdRGB);
        m_filter->setEnabled(lcd);
        updatePreview();
    });
    connect(m_hinting, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewPane::updatePreview);
    connect(m_filter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewPane::updatePreview);
    connect(m_darkening, &QCheckBox::toggled, this, &PreviewPane::updatePreview);
    connect(m_embolden, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewPane::updatePreview);
    connect(m_gamma, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewPane::updatePreview);
    rebuildAntialiasOptions();
    updateBackend(0);

    // Observe clicks on the actual controls/viewport while allowing their
    // original mouse behavior (dragging, selecting text, etc.) to continue.
    for (QWidget *child : findChildren<QWidget *>())
        child->installEventFilter(this);
}

void PreviewPane::setFontFamily(const QString &family, const QString &file)
{
    if (family.isEmpty()) return;
    m_family = family;
    m_file = file;
    m_updating = true;
    m_styleCombo->clear();
    QFontDatabase db;
    const QStringList styles = db.styles(family);
    m_styleCombo->addItems(styles);
    m_style = styles.value(0);
    m_styleCombo->setCurrentIndex(0);
    const bool bold = db.bold(m_family, m_style);
    const bool italic = db.italic(m_family, m_style);
    m_bold->setChecked(bold);
    m_italic->setChecked(italic);
    m_updating = false;
    updatePreview();
}

void PreviewPane::setSampleText(const QString &text)
{
    m_preview->setSampleText(text);
}

void PreviewPane::setActive(bool active)
{
    m_active = active;
    QFont headingFont = m_heading->font();
    headingFont.setBold(active);
    m_heading->setFont(headingFont);
}

int PreviewPane::naturalControlsHeight() const
{
    return m_controls->layout()->sizeHint().height();
}

void PreviewPane::setAlignedControlsHeight(int height)
{
    if (height < 0) {
        m_controls->setMinimumHeight(0);
        m_controls->setMaximumHeight(QWIDGETSIZE_MAX);
    } else if (m_controls->minimumHeight() != height || m_controls->maximumHeight() != height) {
        m_controls->setFixedHeight(height);
    }
}

bool PreviewPane::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress &&
        static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton && !m_active)
        emit activated(this);
    return QWidget::eventFilter(watched, event);
}

void PreviewPane::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_active)
        emit activated(this);
    QWidget::mousePressEvent(event);
}

void PreviewPane::setBackend(RenderBackend backend)
{
    selectChoice(m_backend, static_cast<int>(backend));
}

void PreviewPane::updateBackend(int)
{
    m_updating = true;
    rebuildAntialiasOptions();
    m_updating = false;
    const bool freeType = static_cast<RenderBackend>(m_backend->currentData().toInt()) == RenderBackend::FreeType;
    m_advanced->setVisible(freeType);
    emit controlsHeightChanged();
    m_filter->setEnabled(freeType && int(m_antialias->currentData().toInt()) >= int(AntialiasMode::LcdRGB));
    updatePreview();
}

void PreviewPane::rebuildAntialiasOptions()
{
    QSignalBlocker blocker(m_antialias);
    m_antialias->clear();
    if (static_cast<RenderBackend>(m_backend->currentData().toInt()) == RenderBackend::FreeType) {
        addChoice(m_antialias, tr("Grayscale"), AntialiasMode::Gray);
        addChoice(m_antialias, tr("Monochrome"), AntialiasMode::Mono);
        addChoice(m_antialias, tr("LCD RGB"), AntialiasMode::LcdRGB);
        addChoice(m_antialias, tr("LCD BGR"), AntialiasMode::LcdBGR);
        addChoice(m_antialias, tr("LCD vertical RGB"), AntialiasMode::LcdVRGB);
        addChoice(m_antialias, tr("LCD vertical BGR"), AntialiasMode::LcdVBGR);
        selectChoice(m_antialias, static_cast<int>(m_ftAntialias));
    } else {
        addChoice(m_antialias, tr("System default"), AntialiasMode::System);
        addChoice(m_antialias, tr("Grayscale preferred"), AntialiasMode::Gray);
        addChoice(m_antialias, tr("Monochrome preferred"), AntialiasMode::Mono);
        addChoice(m_antialias, tr("Allow subpixel (system layout)"), AntialiasMode::LcdRGB);
        selectChoice(m_antialias, static_cast<int>(m_qtAntialias));
    }
}

RenderConfig PreviewPane::renderConfig() const
{
    RenderConfig cfg;
    cfg.backend = static_cast<RenderBackend>(m_backend->currentData().toInt());
    cfg.antialias = static_cast<AntialiasMode>(m_antialias->currentData().toInt());
    cfg.hinting = static_cast<HintMode>(m_hinting->currentData().toInt());
    cfg.lcdFilter = static_cast<LcdFilter>(m_filter->currentData().toInt());
    cfg.stemDarkening = m_darkening->isChecked();
    cfg.embolden = m_embolden->value();
    cfg.gamma = m_gamma->value();
    cfg.fontFile = m_file;
    return cfg;
}

void PreviewPane::updatePreview()
{
    if (m_updating || m_family.isEmpty()) return;
    QFontDatabase db;
    QFont font = db.font(m_family, m_style, m_size->value());
    font.setBold(m_bold->isChecked());
    font.setItalic(m_italic->isChecked());
    font.setUnderline(m_underline->isChecked());
    font.setStrikeOut(m_strike->isChecked());
    const RenderConfig config = renderConfig();
    switch (config.hinting) {
    case HintMode::None: font.setHintingPreference(QFont::PreferNoHinting); break;
    case HintMode::Light: font.setHintingPreference(QFont::PreferVerticalHinting); break;
    case HintMode::Full: font.setHintingPreference(QFont::PreferFullHinting); break;
    default: font.setHintingPreference(QFont::PreferDefaultHinting); break;
    }
    int strategy = int(QFont::PreferDefault);
    if (config.antialias == AntialiasMode::Mono)
        strategy = int(QFont::NoAntialias);
    else if (config.antialias != AntialiasMode::System)
        strategy = int(QFont::PreferAntialias);
    if (config.antialias == AntialiasMode::Gray)
        strategy |= int(QFont::NoSubpixelAntialias);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(strategy));

    m_preview->setPreviewFont(font);
    m_preview->setStyleName(m_style);
    m_preview->setRendering(config);
    const QFontInfo info(font);
    m_info->setText(tr("%1 · %2 · %3 pt · %4")
        .arg(m_family.toHtmlEscaped(), m_style.toHtmlEscaped())
        .arg(m_size->value())
        .arg(config.backend == RenderBackend::Qt ? tr("Qt") : tr("FreeType")));
    m_info->setToolTip(tr("Qt-resolved pixel size: %1; exact match: %2")
                       .arg(info.pixelSize()).arg(info.exactMatch() ? tr("yes") : tr("no")));
}

void PreviewPane::copyFrom(const PreviewPane &other)
{
    setFontFamily(other.m_family, other.m_file);
    m_updating = true;
    m_styleCombo->setCurrentText(other.m_styleCombo->currentText());
    m_style = other.m_style;
    m_size->setValue(other.m_size->value());
    m_bold->setChecked(other.m_bold->isChecked());
    m_italic->setChecked(other.m_italic->isChecked());
    m_underline->setChecked(other.m_underline->isChecked());
    m_strike->setChecked(other.m_strike->isChecked());
    m_qtAntialias = other.m_qtAntialias;
    m_ftAntialias = other.m_ftAntialias;
    selectChoice(m_hinting, other.m_hinting->currentData().toInt());
    selectChoice(m_filter, other.m_filter->currentData().toInt());
    m_darkening->setChecked(other.m_darkening->isChecked());
    m_embolden->setValue(other.m_embolden->value());
    m_gamma->setValue(other.m_gamma->value());
    m_updating = false;
    setBackend(static_cast<RenderBackend>(other.m_backend->currentData().toInt()));
    updatePreview();
}
