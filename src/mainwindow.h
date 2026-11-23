// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "previewpane.h"

#include <QMainWindow>
#include <QFontDatabase>
#include <QHash>

class QAction;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    bool loadFontFile(const QString &filename, QString *errorMessage = nullptr);
    void setSplitPreview(bool split);
    void setPrimaryBackend(RenderBackend backend);

private:
    void setupUi();
    void populateFonts();
    void onSearchTextChanged(const QString &text);
    void onFontSelected(QListWidgetItem *item);
    void activatePane(PreviewPane *pane);
    void openFontFile();
    void about();

    QFontDatabase m_fontDb;
    QHash<QString, QString> m_loadedFontFiles;
    QListWidget *m_fontList = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QTextEdit *m_sampleEdit = nullptr;
    QSplitter *m_paneSplitter = nullptr;
    QAction *m_splitAction = nullptr;
    PreviewPane *m_paneA = nullptr;
    PreviewPane *m_paneB = nullptr;
    PreviewPane *m_activePane = nullptr;
};
