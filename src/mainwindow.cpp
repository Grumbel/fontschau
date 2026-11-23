// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QFontInfo>
#include <QScrollArea>
#include <QFrame>
#include <QIcon>
#include <QFileDialog>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    populateFonts();
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Fontschau – Font Viewer"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/fontschau.svg")));
    resize(1100, 700);

    // Menu bar
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *openFontAction = fileMenu->addAction(tr("&Open Font…"));
    openFontAction->setShortcut(QKeySequence::Open);
    connect(openFontAction, &QAction::triggered, this, &MainWindow::openFontFile);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = helpMenu->addAction(tr("&About Fontschau"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);

    // Central widget with splitter
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    mainLayout->addWidget(splitter);

    // Left panel: search + font list
    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr("Search fonts…"));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    leftLayout->addWidget(m_searchEdit);

    m_fontList = new QListWidget;
    m_fontList->setUniformItemSizes(true);
    m_fontList->setSortingEnabled(true);
    connect(m_fontList, &QListWidget::itemClicked, this, &MainWindow::onFontSelected);
    connect(m_fontList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (current)
                    onFontSelected(current);
            });
    leftLayout->addWidget(m_fontList);

    splitter->addWidget(leftPanel);

    // Right panel: controls + preview
    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Controls group
    auto *controlsGroup = new QGroupBox(tr("Font Properties"));
    auto *controlsLayout = new QHBoxLayout(controlsGroup);

    // Style
    auto *styleLabel = new QLabel(tr("Style:"));
    m_styleCombo = new QComboBox;
    m_styleCombo->setMinimumWidth(140);
    connect(m_styleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStyleChanged);
    controlsLayout->addWidget(styleLabel);
    controlsLayout->addWidget(m_styleCombo);

    // Size
    auto *sizeLabel = new QLabel(tr("Size:"));
    m_sizeSpin = new QSpinBox;
    m_sizeSpin->setRange(6, 200);
    m_sizeSpin->setValue(24);
    m_sizeSpin->setSuffix(tr(" pt"));
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onSizeChanged);
    controlsLayout->addWidget(sizeLabel);
    controlsLayout->addWidget(m_sizeSpin);

    m_sizeSlider = new QSlider(Qt::Horizontal);
    m_sizeSlider->setRange(6, 120);
    m_sizeSlider->setValue(24);
    m_sizeSlider->setMaximumWidth(180);
    connect(m_sizeSlider, &QSlider::valueChanged, m_sizeSpin, &QSpinBox::setValue);
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_sizeSlider, &QSlider::setValue);
    controlsLayout->addWidget(m_sizeSlider);

    // Style toggles
    m_boldCheck = new QCheckBox(tr("Bold"));
    m_italicCheck = new QCheckBox(tr("Italic"));
    m_underlineCheck = new QCheckBox(tr("Underline"));
    m_strikeoutCheck = new QCheckBox(tr("Strikeout"));
    connect(m_boldCheck, &QCheckBox::toggled, this, &MainWindow::onBoldToggled);
    connect(m_italicCheck, &QCheckBox::toggled, this, &MainWindow::onItalicToggled);
    connect(m_underlineCheck, &QCheckBox::toggled, this, &MainWindow::onUnderlineToggled);
    connect(m_strikeoutCheck, &QCheckBox::toggled, this, &MainWindow::onStrikeoutToggled);

    controlsLayout->addWidget(m_boldCheck);
    controlsLayout->addWidget(m_italicCheck);
    controlsLayout->addWidget(m_underlineCheck);
    controlsLayout->addWidget(m_strikeoutCheck);
    controlsLayout->addStretch();

    rightLayout->addWidget(controlsGroup);

    // Public Qt font rendering preferences. More advanced FreeType settings
    // (LCD filter, stem darkening, gamma) aren't exposed through Qt's API.
    auto *renderGroup = new QGroupBox(tr("Rendering"));
    auto *renderLayout = new QHBoxLayout(renderGroup);
    renderLayout->addWidget(new QLabel(tr("Antialiasing:")));
    m_antialiasCombo = new QComboBox;
    m_antialiasCombo->addItem(tr("System default"), int(QFont::PreferDefault));
    m_antialiasCombo->addItem(tr("Prefer on"), int(QFont::PreferAntialias));
    m_antialiasCombo->addItem(tr("Off"), int(QFont::NoAntialias));
    m_antialiasCombo->setToolTip(tr("Antialiasing is a preference; Qt and the platform may override it."));
    connect(m_antialiasCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updatePreview);
    renderLayout->addWidget(m_antialiasCombo);

    renderLayout->addWidget(new QLabel(tr("Hinting:")));
    m_hintingCombo = new QComboBox;
    m_hintingCombo->addItem(tr("System default"), int(QFont::PreferDefaultHinting));
    m_hintingCombo->addItem(tr("None"), int(QFont::PreferNoHinting));
    m_hintingCombo->addItem(tr("Vertical (light)"), int(QFont::PreferVerticalHinting));
    m_hintingCombo->addItem(tr("Full"), int(QFont::PreferFullHinting));
    m_hintingCombo->setToolTip(tr("Qt font hinting preference (platform support varies)."));
    connect(m_hintingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updatePreview);
    renderLayout->addWidget(m_hintingCombo);

    m_subpixelCheck = new QCheckBox(tr("Allow subpixel AA"));
    m_subpixelCheck->setChecked(true);
    m_subpixelCheck->setToolTip(tr("Disable subpixel antialiasing if possible; enabling this does not force it on."));
    connect(m_subpixelCheck, &QCheckBox::toggled, this, &MainWindow::updatePreview);
    renderLayout->addWidget(m_subpixelCheck);
    renderLayout->addStretch();
    rightLayout->addWidget(renderGroup);

    // Sample text input
    auto *sampleGroup = new QGroupBox(tr("Sample Text"));
    auto *sampleLayout = new QVBoxLayout(sampleGroup);
    m_sampleEdit = new QTextEdit;
    m_sampleEdit->setMaximumHeight(80);
    m_sampleEdit->setPlaceholderText(tr("Enter custom sample text…"));
    m_sampleEdit->setPlainText(
        tr("The quick brown fox jumps over the lazy dog.\n"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
           "abcdefghijklmnopqrstuvwxyz\n"
           "0123456789 !@#$%^&*()"));
    connect(m_sampleEdit, &QTextEdit::textChanged, this, &MainWindow::onSampleTextChanged);
    sampleLayout->addWidget(m_sampleEdit);
    rightLayout->addWidget(sampleGroup);

    // Preview
    auto *previewGroup = new QGroupBox(tr("Preview"));
    auto *previewLayout = new QVBoxLayout(previewGroup);
    m_preview = new FontPreview;
    auto *scroll = new QScrollArea;
    scroll->setWidget(m_preview);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    previewLayout->addWidget(scroll);
    rightLayout->addWidget(previewGroup, 1);

    // Info label
    m_infoLabel = new QLabel;
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rightLayout->addWidget(m_infoLabel);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({280, 800});
}

void MainWindow::populateFonts()
{
    m_fontList->clear();
    const QStringList families = m_fontDb.families();
    for (const QString &family : families) {
        auto *item = new QListWidgetItem(family);
        // Use the font itself for the list item so it previews a little
        QFont f(family);
        f.setPointSize(11);
        item->setFont(f);
        m_fontList->addItem(item);
    }
    statusBar()->showMessage(tr("%1 font families available").arg(families.size()));

    if (m_fontList->count() > 0) {
        m_fontList->setCurrentRow(0);
    }
}

bool MainWindow::loadFontFile(const QString &filename, QString *errorMessage)
{
    const QFileInfo file(filename);
    if (!file.isFile() || !file.isReadable()) {
        if (errorMessage)
            *errorMessage = tr("Cannot read font file: %1").arg(filename);
        return false;
    }

    const int fontId = QFontDatabase::addApplicationFont(file.absoluteFilePath());
    if (fontId < 0) {
        if (errorMessage)
            *errorMessage = tr("Unsupported or invalid font file: %1").arg(filename);
        return false;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        QFontDatabase::removeApplicationFont(fontId);
        if (errorMessage)
            *errorMessage = tr("No font families found in: %1").arg(filename);
        return false;
    }

    populateFonts();
    m_searchEdit->clear();
    const auto matchingItems = m_fontList->findItems(families.first(), Qt::MatchExactly);
    if (!matchingItems.isEmpty()) {
        m_fontList->setCurrentItem(matchingItems.first());
        m_fontList->scrollToItem(matchingItems.first());
    }
    statusBar()->showMessage(tr("Loaded %1 (%2)")
                                 .arg(file.fileName(), families.join(QStringLiteral(", "))));
    return true;
}

void MainWindow::openFontFile()
{
    const QString filename = QFileDialog::getOpenFileName(
        this, tr("Open Font"), QString(),
        tr("Font files (*.ttf *.otf *.ttc *.otc);;All files (*)"));
    if (filename.isEmpty())
        return;

    QString error;
    if (!loadFontFile(filename, &error))
        QMessageBox::warning(this, tr("Cannot Open Font"), error);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    const QString needle = text.trimmed();
    for (int i = 0; i < m_fontList->count(); ++i) {
        QListWidgetItem *item = m_fontList->item(i);
        const bool match = needle.isEmpty() ||
                           item->text().contains(needle, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

void MainWindow::onFontSelected(QListWidgetItem *item)
{
    if (!item)
        return;

    m_currentFamily = item->text();
    m_styleCombo->blockSignals(true);
    m_styleCombo->clear();

    const QStringList styles = m_fontDb.styles(m_currentFamily);
    for (const QString &style : styles) {
        m_styleCombo->addItem(style);
    }
    m_styleCombo->blockSignals(false);

    if (!styles.isEmpty()) {
        m_currentStyle = styles.first();
        m_styleCombo->setCurrentIndex(0);
    } else {
        m_currentStyle.clear();
    }

    // Sync checkboxes with available style info where possible
    const bool isBold = m_fontDb.bold(m_currentFamily, m_currentStyle);
    const bool isItalic = m_fontDb.italic(m_currentFamily, m_currentStyle);
    m_boldCheck->blockSignals(true);
    m_italicCheck->blockSignals(true);
    m_boldCheck->setChecked(isBold);
    m_italicCheck->setChecked(isItalic);
    m_boldCheck->blockSignals(false);
    m_italicCheck->blockSignals(false);

    updatePreview();
}

void MainWindow::onStyleChanged(int index)
{
    if (index < 0)
        return;
    m_currentStyle = m_styleCombo->itemText(index);

    const bool isBold = m_fontDb.bold(m_currentFamily, m_currentStyle);
    const bool isItalic = m_fontDb.italic(m_currentFamily, m_currentStyle);
    m_boldCheck->blockSignals(true);
    m_italicCheck->blockSignals(true);
    m_boldCheck->setChecked(isBold);
    m_italicCheck->setChecked(isItalic);
    m_boldCheck->blockSignals(false);
    m_italicCheck->blockSignals(false);

    updatePreview();
}

void MainWindow::onSizeChanged(int size)
{
    Q_UNUSED(size);
    updatePreview();
}

void MainWindow::onSampleTextChanged()
{
    updatePreview();
}

void MainWindow::onBoldToggled(bool checked)
{
    Q_UNUSED(checked);
    updatePreview();
}

void MainWindow::onItalicToggled(bool checked)
{
    Q_UNUSED(checked);
    updatePreview();
}

void MainWindow::onUnderlineToggled(bool checked)
{
    Q_UNUSED(checked);
    updatePreview();
}

void MainWindow::onStrikeoutToggled(bool checked)
{
    Q_UNUSED(checked);
    updatePreview();
}

QFont MainWindow::selectedFont() const
{
    QFont font = m_fontDb.font(m_currentFamily, m_currentStyle, m_sizeSpin->value());
    // Override with checkbox state so the user can force styles.
    font.setBold(m_boldCheck->isChecked());
    font.setItalic(m_italicCheck->isChecked());
    font.setUnderline(m_underlineCheck->isChecked());
    font.setStrikeOut(m_strikeoutCheck->isChecked());
    font.setHintingPreference(static_cast<QFont::HintingPreference>(
        m_hintingCombo->currentData().toInt()));

    int strategy = m_antialiasCombo->currentData().toInt();
    if (!m_subpixelCheck->isChecked())
        strategy |= int(QFont::NoSubpixelAntialias);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(strategy));
    return font;
}

void MainWindow::applyFontSettings()
{
    m_preview->setPreviewFont(selectedFont());
    m_preview->setSampleText(m_sampleEdit->toPlainText());
}

void MainWindow::updatePreview()
{
    if (m_currentFamily.isEmpty())
        return;

    applyFontSettings();

    // Build info string
    QFontInfo info(selectedFont());

    QStringList lines;
    lines << tr("<b>Family:</b> %1").arg(m_currentFamily.toHtmlEscaped());
    lines << tr("<b>Style:</b> %1").arg(m_currentStyle.toHtmlEscaped());
    lines << tr("<b>Point size:</b> %1").arg(m_sizeSpin->value());
    lines << tr("<b>Pixel size:</b> %1").arg(info.pixelSize());
    lines << tr("<b>Weight:</b> %1").arg(info.weight());
    lines << tr("<b>Exact match:</b> %1").arg(info.exactMatch() ? tr("yes") : tr("no"));
    lines << tr("<b>Fixed pitch:</b> %1").arg(info.fixedPitch() ? tr("yes") : tr("no"));
    lines << tr("<b>Style hint:</b> %1").arg(static_cast<int>(info.styleHint()));

    m_infoLabel->setText(lines.join(" &nbsp;|&nbsp; "));
    statusBar()->showMessage(tr("%1 – %2 @ %3 pt")
                                 .arg(m_currentFamily, m_currentStyle)
                                 .arg(m_sizeSpin->value()));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Fontschau"),
                       tr("<h3>Fontschau</h3>"
                          "<p>A simple font viewer built with Qt6.</p>"
                          "<p>Version %1</p>"
                          "<p>Browse installed fonts, adjust size and style, "
                          "and preview custom sample text.</p>"
                          "<p>Author: Ingo Ruhnke &lt;grumbel@gmail.com&gt;<br>"
                          "Co-authored-by: Grok &lt;grok@x.ai&gt;</p>")
                           .arg(QApplication::applicationVersion()));
}