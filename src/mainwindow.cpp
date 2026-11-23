// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    populateFonts();
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Fontschau – Font Viewer"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/fontschau.svg")));
    resize(1200, 830);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *openFontAction = fileMenu->addAction(tr("&Open Font…"));
    openFontAction->setShortcut(QKeySequence::Open);
    connect(openFontAction, &QAction::triggered, this, &MainWindow::openFontFile);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    m_splitAction = viewMenu->addAction(tr("&Split Preview"));
    m_splitAction->setObjectName(QStringLiteral("splitPreviewAction"));
    m_splitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+\\")));
    m_splitAction->setCheckable(true);
    connect(m_splitAction, &QAction::toggled, this, &MainWindow::setSplitPreview);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = helpMenu->addAction(tr("&About Fontschau"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    auto *outer = new QSplitter(Qt::Horizontal, central);
    mainLayout->addWidget(outer);

    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr("Search fonts…"));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    leftLayout->addWidget(m_searchEdit);
    m_fontList = new QListWidget;
    m_fontList->setObjectName(QStringLiteral("fontList"));
    m_fontList->setUniformItemSizes(true);
    m_fontList->setSortingEnabled(true);
    connect(m_fontList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) { onFontSelected(current); });
    leftLayout->addWidget(m_fontList);
    outer->addWidget(leftPanel);

    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto *sampleGroup = new QGroupBox(tr("Shared sample text"));
    auto *sampleLayout = new QVBoxLayout(sampleGroup);
    m_sampleEdit = new QTextEdit;
    m_sampleEdit->setMaximumHeight(80);
    m_sampleEdit->setPlainText(
        tr("The quick brown fox jumps over the lazy dog.\n"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
           "abcdefghijklmnopqrstuvwxyz\n"
           "0123456789 !@#$%^&*()\n"
           "Fünf müde Füchse hüpfen über zwölf süße Äpfel. Ää Öö Üü ß ẞ\n"
           "Western/Nordic: Àà Áá Ââ Ãã Åå Ææ Çç Èè Éé Êê Ëë Íí Ññ Øø Œœ Óó Úú Ýý Ðð Þþ\n"
           "Central/Eastern: Ąą Ćć Čč Ďď Ęę Ěě Łł Ńń Řř Śś Šš Ťť Ůů Źź Żż Žž\n"
           "Other accents: Ăă Șș Țț Őő Űű Ğğ İı Şş Đđ Āā Ēē Īī Ūū\n"
           "Cyrillic: Съешь ещё этих мягких французских булок, да выпей чаю.\n"
           "Other Cyrillic: Її Єє Іі Ґґ Ўў Ђђ Јј Љљ Њњ Ћћ Џџ\n"
           "😀 😃 😎 🥳 🤖 🦊 🌍 🚀 🎨 🎉 🍕"));
    sampleLayout->addWidget(m_sampleEdit);
    rightLayout->addWidget(sampleGroup);

    m_paneSplitter = new QSplitter(Qt::Horizontal);
    m_paneSplitter->setObjectName(QStringLiteral("previewSplitter"));
    m_paneSplitter->setChildrenCollapsible(false);
    m_paneA = new PreviewPane(tr("Pane A"), false, m_paneSplitter);
    m_paneSplitter->addWidget(m_paneA);
    rightLayout->addWidget(m_paneSplitter, 1);
    outer->addWidget(rightPanel);
    outer->setStretchFactor(0, 0);
    outer->setStretchFactor(1, 1);
    outer->setSizes({250, 950});

    connect(m_paneA, &PreviewPane::activated, this, &MainWindow::activatePane);
    connect(m_paneA, &PreviewPane::controlsHeightChanged, this, &MainWindow::alignPanePreviews);
    connect(m_sampleEdit, &QTextEdit::textChanged, this, [this] {
        const QString text = m_sampleEdit->toPlainText();
        m_paneA->setSampleText(text);
        if (m_paneB) m_paneB->setSampleText(text);
    });
    m_paneA->setSampleText(m_sampleEdit->toPlainText());
    activatePane(m_paneA);
}

void MainWindow::populateFonts()
{
    const QString selected = m_activePane ? m_activePane->fontFamily() : QString();
    QSignalBlocker blocker(m_fontList);
    m_fontList->clear();
    const QStringList families = m_fontDb.families();
    for (const QString &family : families) {
        auto *item = new QListWidgetItem(family);
        QFont f(family);
        f.setPointSize(11);
        item->setFont(f);
        m_fontList->addItem(item);
    }
    QListWidgetItem *target = nullptr;
    const auto found = m_fontList->findItems(selected, Qt::MatchExactly);
    if (!found.isEmpty()) target = found.first();
    else if (m_fontList->count() > 0) target = m_fontList->item(0);
    if (target) m_fontList->setCurrentItem(target);
    blocker.unblock();
    if (target && m_activePane && m_activePane->fontFamily().isEmpty())
        onFontSelected(target);
    statusBar()->showMessage(tr("%1 font families available").arg(families.size()));
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    const QString needle = text.trimmed();
    for (int i = 0; i < m_fontList->count(); ++i) {
        QListWidgetItem *item = m_fontList->item(i);
        item->setHidden(!needle.isEmpty() && !item->text().contains(needle, Qt::CaseInsensitive));
    }
}

void MainWindow::onFontSelected(QListWidgetItem *item)
{
    if (!item || !m_activePane) return;
    m_activePane->setFontFamily(item->text(), m_loadedFontFiles.value(item->text()));
    statusBar()->showMessage(tr("%1 selected for comparison pane").arg(item->text()));
}

void MainWindow::activatePane(PreviewPane *pane)
{
    m_activePane = pane;
    m_paneA->setActive(pane == m_paneA);
    if (m_paneB) m_paneB->setActive(pane == m_paneB);
    const auto items = m_fontList->findItems(pane->fontFamily(), Qt::MatchExactly);
    if (!items.isEmpty()) {
        QSignalBlocker blocker(m_fontList);
        m_fontList->setCurrentItem(items.first());
        m_fontList->scrollToItem(items.first());
    }
    alignPanePreviews();
}

void MainWindow::alignPanePreviews()
{
    if (!m_paneB) {
        m_paneA->setAlignedControlsHeight(-1);
        return;
    }
    // Qt and FreeType display different amounts of configuration UI. Reserve
    // the larger natural height on both sides to align the preview scroll areas.
    const int height = std::max(m_paneA->naturalControlsHeight(),
                                m_paneB->naturalControlsHeight());
    m_paneA->setAlignedControlsHeight(height);
    m_paneB->setAlignedControlsHeight(height);
}

void MainWindow::setSplitPreview(bool split)
{
    if (split && !m_paneB) {
        m_paneB = new PreviewPane(tr("Pane B"), true, m_paneSplitter);
        m_paneB->copyFrom(*m_paneA);
        m_paneB->setSampleText(m_sampleEdit->toPlainText());
        m_paneSplitter->addWidget(m_paneB);
        m_paneSplitter->setSizes({500, 500});
        connect(m_paneB, &PreviewPane::activated, this, &MainWindow::activatePane);
        connect(m_paneB, &PreviewPane::controlsHeightChanged, this, &MainWindow::alignPanePreviews);
        connect(m_paneB, &PreviewPane::closeRequested, this,
                [this] (PreviewPane *) { setSplitPreview(false); });
        activatePane(m_paneB);
    } else if (!split && m_paneB) {
        activatePane(m_paneA);
        PreviewPane *closing = m_paneB;
        m_paneB = nullptr;
        closing->hide();
        closing->setParent(nullptr); // Removes it from the splitter immediately.
        closing->deleteLater(); // Safe when invoked by the pane's own button.
    }
    alignPanePreviews();
    if (m_splitAction && m_splitAction->isChecked() != split) {
        QSignalBlocker blocker(m_splitAction);
        m_splitAction->setChecked(split);
    }
}

void MainWindow::setPrimaryBackend(RenderBackend backend)
{
    m_paneA->setBackend(backend);
}

bool MainWindow::loadFontFile(const QString &filename, QString *errorMessage)
{
    const QFileInfo file(filename);
    if (!file.isFile() || !file.isReadable()) {
        if (errorMessage) *errorMessage = tr("Cannot read font file: %1").arg(filename);
        return false;
    }
    const int fontId = QFontDatabase::addApplicationFont(file.absoluteFilePath());
    if (fontId < 0) {
        if (errorMessage) *errorMessage = tr("Unsupported or invalid font file: %1").arg(filename);
        return false;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        QFontDatabase::removeApplicationFont(fontId);
        if (errorMessage) *errorMessage = tr("No font families found in: %1").arg(filename);
        return false;
    }
    for (const QString &family : families)
        m_loadedFontFiles.insert(family, file.absoluteFilePath());
    populateFonts();
    m_searchEdit->clear();
    const auto matchingItems = m_fontList->findItems(families.first(), Qt::MatchExactly);
    if (!matchingItems.isEmpty()) {
        m_fontList->setCurrentItem(matchingItems.first());
        // Selecting a file twice must still assign it to the active pane.
        onFontSelected(matchingItems.first());
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
    if (filename.isEmpty()) return;
    QString error;
    if (!loadFontFile(filename, &error))
        QMessageBox::warning(this, tr("Cannot Open Font"), error);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Fontschau"),
        tr("<h3>Fontschau</h3>"
           "<p>Compare fonts and rasterizer settings using Qt or FreeType/Fontconfig.</p>"
           "<p>Version %1</p>"
           "<p>Author: Ingo Ruhnke &lt;grumbel@gmail.com&gt;</p>")
            .arg(QApplication::applicationVersion()));
}
