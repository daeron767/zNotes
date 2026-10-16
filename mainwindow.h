#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QSessionManager>
#include <QTimer>

#include "notelist.h"
#include "settings.h"
#include "toolbaraction.h"

class NoteCreateWidget;
class QCloseEvent;
class QComboBox;
class QHideEvent;
class QMoveEvent;
class QPrinter;
class QShowEvent;
class QTextEdit;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void SetCheckedInverse(bool checked);
    void NewNote();
    void NewNotePlain();
    void NewNoteHTML();
    void NewNoteTODO();

    void PreviousNote();
    void NextNote();
    void ToNote(int n);

    void CopyNote();

    void notePrint();
    void notePrintPreview();
    void notePrintPdf();

    void currentNoteChanged(int old_index, int new_index);

    void trayActivated(QSystemTrayIcon::ActivationReason reason);

    void commandMenu();
    void showAboutDialog();
    void showPrefDialog();
    void showSearchBar(bool show = true);

    void formatBold();
    void formatItalic();
    void formatStrikeout();
    void formatUnderline();
    void formatTextColor();
    void formatSerif();
    void formatZoomIn();
    void formatZoomOut();

    void actions_changed();
    void icons_size_changed();

    void cmd_changed();
    void cmdExec(const QString &command);
    void edit_command_list();

    void windowStateChanged();
    void warningSettingsChanged();

protected:
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void changeEvent(QEvent *e) override;

private slots:
    void applyNoteFontToAll();
    void findNext();
    void findPrev();
    void formatChanged(const QTextCharFormat &format);
    void on_actionFontSerif_triggered(bool checked);
    void on_actionHtmlSize_activated(int index);
    void on_btSearchClose_clicked();
    void on_edSearch_returnPressed();
    void on_edSearch_textChanged(const QString &text);
    void on_tbCase_toggled(bool checked);
    void on_tbHighlightAll_toggled(bool checked);
    void performSearch();
    void printPreview(QPrinter *printer);
    void toggleReadOnly();
    void toggleSearchMode(bool searchAll);

private:
    Ui::MainWindow *ui{nullptr};

    NoteList *notes{nullptr};
    NoteCreateWidget *note_create_widget{nullptr};

    QSystemTrayIcon tray;
    QMenu cmenu;
    QMenu cmd_menu;

    QAction *actions[itemMax]{nullptr};
    QAction *actShow{nullptr};
    QAction *actHide{nullptr};
    QAction *actReadOnly{nullptr};
    QComboBox *comboHtmlSize{nullptr};
    QTextEdit *currentTextEdit() const;
    QTimer *autoSaveTimer{nullptr};

    bool isNoteSupportsPrinting(const Note *note) const;
    bool m_readOnly = false;
    bool m_searchAllNotes = false;

    void findInCurrentNote(bool next, bool backward = false);
    void highlightAllMatches(bool enable);
    void purgeOldBackupFiles();
    void saveAllNotes();
    void Search(bool next);
    void setupUiComponents();
    void setupActions();
    void setupShortcuts();
    void setupTray();
    void setupConnections();
    void updateFormatActions(const QTextCharFormat &fmt);
};

#endif // MAINWINDOW_H
