#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFontDatabase>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSlider>

#include "fontpreview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onFontSelected(QListWidgetItem *item);
    void onSearchTextChanged(const QString &text);
    void onStyleChanged(int index);
    void onSizeChanged(int size);
    void onSampleTextChanged();
    void onBoldToggled(bool checked);
    void onItalicToggled(bool checked);
    void onUnderlineToggled(bool checked);
    void onStrikeoutToggled(bool checked);
    void updatePreview();
    void populateFonts();
    void about();

private:
    void setupUi();
    void applyFontSettings();

    QFontDatabase m_fontDb;
    QString m_currentFamily;
    QString m_currentStyle;

    // UI elements
    QListWidget *m_fontList = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_styleCombo = nullptr;
    QSpinBox *m_sizeSpin = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QCheckBox *m_boldCheck = nullptr;
    QCheckBox *m_italicCheck = nullptr;
    QCheckBox *m_underlineCheck = nullptr;
    QCheckBox *m_strikeoutCheck = nullptr;
    QTextEdit *m_sampleEdit = nullptr;
    FontPreview *m_preview = nullptr;
    QLabel *m_infoLabel = nullptr;
};

#endif // MAINWINDOW_H