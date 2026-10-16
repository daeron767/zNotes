#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "configdialog.h"
#include "aboutDialog.h"
#include "htmlnote.h"
#include "notecreatewidget.h"
#include "shared.h"
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QKeySequence>
#include <QMessageBox>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#if defined(Q_OS_WIN)
    #define TRAY_ICON_FILE_NAME ":/res/znotes.png"
#elif defined(Q_OS_MAC)
    #define TRAY_ICON_FILE_NAME ":/res/znotes32_bw.png"
#else
    #define TRAY_ICON_FILE_NAME ":/res/znotes32.png"
#endif

Settings settings;
static inline QAction* generateAction(int item_id) {
    auto item = static_cast<item_enum>(item_id);
    ToolbarAction toolbar_action(item, settings.getIconsUseSystemTheme());
    auto *action = new QAction(toolbar_action.icon(), toolbar_action.text(), nullptr);
    action->setCheckable(toolbar_action.isCheckable());
    return action;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Auto-save timer setup (1-minute interval)
    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setInterval(60 * 1000);
    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::saveAllNotes);
    autoSaveTimer->start();
    // Core note list & widget initialization
    ui->wSearch->hide();
    notes = new NoteList(ui->centralWidget);
    ui->layout->addWidget(notes->getWidget());
    // Restore window geometry & settings state
    restoreGeometry(settings.getDialogGeometry());
    restoreState(settings.getDialogState());
    windowStateChanged();
    // Modularized initialization pipeline
    setupUiComponents();
    setupActions();
    setupShortcuts();
    setupTray();
    setupConnections();
    // Initial state updates & window launch state
    actions_changed();
    icons_size_changed();
    cmd_changed();
    if (!settings.getHideStart())
        show();
    currentNoteChanged(-1, notes->currentIndex());
}

void MainWindow::setupUiComponents()
{
    comboHtmlSize = new QComboBox(this);
    comboHtmlSize->addItems({
        tr("x-small"), tr("small"), tr("default"),
        tr("large"), tr("x-large"), tr("xx-large")
    });
    comboHtmlSize->setCurrentIndex(2); // Index 2: Default baseline
}

void MainWindow::setupActions()
{
    for (int i = 0; i < itemMax; ++i)
        actions[i] = generateAction(i);
    actShow = new QAction(tr("Show"), this);
    actHide = new QAction(tr("Hide"), this);
    actions[itemAdd]->setCheckable(true);
    // Read-Only Action setup
    actions[itemReadOnly] = new QAction(ToolbarAction(itemReadOnly).icon(),
                                         ToolbarAction(itemReadOnly).text(), this);
    actions[itemReadOnly]->setCheckable(true);
    actions[itemReadOnly]->setChecked(false);

    // Search action state initialization
    if (actions[itemSearch]) {
        actions[itemSearch]->setCheckable(true);
    }

    // Serif action configuration
    if (actions[itemFormatSerif]) {
        actions[itemFormatSerif]->setCheckable(true);
    }
}

void MainWindow::setupShortcuts()
{
    // Application-wide Zoom Shortcuts
    auto *zoomInEqual = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this);
    zoomInEqual->setContext(Qt::ApplicationShortcut);
    connect(zoomInEqual, &QShortcut::activated, this, &MainWindow::formatZoomIn);
    auto *zoomInPlus = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this);
    zoomInPlus->setContext(Qt::ApplicationShortcut);
    connect(zoomInPlus, &QShortcut::activated, this, &MainWindow::formatZoomIn);
    auto *zoomOutMinus = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this);
    zoomOutMinus->setContext(Qt::ApplicationShortcut);
    connect(zoomOutMinus, &QShortcut::activated, this, &MainWindow::formatZoomOut);
    // Global Find/Search Shortcut
    auto *findShortcut = new QShortcut(QKeySequence::Find, this);
    findShortcut->setContext(Qt::ApplicationShortcut);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        showSearchBar(!ui->wSearch->isVisible());
    });
    // Escape Key behavior (Dismiss to Tray or Close Search)
    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escShortcut->setContext(Qt::ApplicationShortcut);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (ui->wSearch->isVisible()) {
            showSearchBar(false);
        } else {
            close(); // Triggers closeEvent, dropping zNotes to tray
        }
    });
    // Rich Text Formatting Shortcuts
    auto addFormatShortcut = [this](const QKeySequence &seq, const QString &label, auto slot) {
        auto *shortcut = new QShortcut(seq, this);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        Shared::addShortcut(shortcut, label);
        connect(shortcut, &QShortcut::activated, this, slot);
    };
    addFormatShortcut(QKeySequence::Bold, tr("Make selected text bold"), &MainWindow::formatBold);
    addFormatShortcut(QKeySequence::Italic, tr("Make selected text italic"), &MainWindow::formatItalic);
    addFormatShortcut(QKeySequence::Underline, tr("Make selected text underline"), &MainWindow::formatUnderline);
    addFormatShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), tr("Make selected text strikeout"), &MainWindow::formatStrikeout);
    addFormatShortcut(QKeySequence::New, tr("New Html Note"), &MainWindow::NewNoteHTML);
}

void MainWindow::setupTray()
{
    cmenu.addAction(actShow);
    cmenu.addAction(actHide);
    cmenu.addSeparator();
    cmenu.addAction(actions[itemAdd]);
    cmenu.addAction(actions[itemRemove]);
    cmenu.addAction(actions[itemRename]);
    cmenu.addSeparator();
    cmenu.addAction(actions[itemSetup]);
    cmenu.addAction(actions[itemInfo]);
    cmenu.addSeparator();
    cmenu.addAction(actions[itemExit]);
    tray.setIcon(QIcon(TRAY_ICON_FILE_NAME));
    tray.setContextMenu(&cmenu);
    tray.show();
}

void MainWindow::setupConnections()
{
    // Note Management Signals
    connect(notes, &NoteList::currentNoteChanged, this, &MainWindow::currentNoteChanged);
    connect(actions[itemAdd], &QAction::triggered, this, &MainWindow::NewNote);
    connect(actions[itemAddText], &QAction::triggered, this, &MainWindow::NewNotePlain);
    connect(actions[itemAddHtml], &QAction::triggered, this, &MainWindow::NewNoteHTML);
    connect(actions[itemAddTodo], &QAction::triggered, this, &MainWindow::NewNoteTODO);
    connect(actions[itemRemove], &QAction::triggered, notes, &NoteList::removeCurrentNote);
    connect(actions[itemRename], &QAction::triggered, notes, &NoteList::renameCurrentNote);
    connect(actions[itemPrev], &QAction::triggered, this, &MainWindow::PreviousNote);
    connect(actions[itemNext], &QAction::triggered, this, &MainWindow::NextNote);
    connect(actions[itemCopy], &QAction::triggered, this, &MainWindow::CopyNote);
    // Application & Dialog Signals
    connect(actions[itemReadOnly], &QAction::triggered, this, &MainWindow::toggleReadOnly);
    connect(actions[itemSetup], &QAction::triggered, this, &MainWindow::showPrefDialog);
    connect(actions[itemInfo], &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(actions[itemRun], &QAction::triggered, this, &MainWindow::commandMenu);
    connect(actions[itemSearch], &QAction::toggled, this, &MainWindow::showSearchBar);
    connect(actions[itemExit], &QAction::triggered, qApp, &QApplication::quit);
    // Rich Text Toolbar Actions
    connect(actions[itemFormatBold], &QAction::triggered, this, &MainWindow::formatBold);
    connect(actions[itemFormatItalic], &QAction::triggered, this, &MainWindow::formatItalic);
    connect(actions[itemFormatStrikeout], &QAction::triggered, this, &MainWindow::formatStrikeout);
    connect(actions[itemFormatUnderline], &QAction::triggered, this, &MainWindow::formatUnderline);
    connect(actions[itemFormatColor], &QAction::triggered, this, &MainWindow::formatTextColor);
    connect(actions[itemFormatSerif], &QAction::triggered, this, &MainWindow::formatSerif);
    if (actions[itemFormatSerif]) {
        connect(actions[itemFormatSerif], &QAction::triggered, this, &MainWindow::on_actionFontSerif_triggered);
    }
    // HTML Size ComboBox
    connect(comboHtmlSize, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_actionHtmlSize_activated);
    // Export & Print Actions
    connect(actions[itemExportPdf], &QAction::triggered, this, &MainWindow::notePrintPdf);
    connect(actions[itemPrintNote], &QAction::triggered, this, &MainWindow::notePrint);
    connect(actions[itemPrintPreviewNote], &QAction::triggered, this, &MainWindow::notePrintPreview);
    // Search Bar Controls
    connect(ui->tbNext, &QToolButton::clicked, this, &MainWindow::findNext);
    connect(ui->tbPrev, &QToolButton::clicked, this, &MainWindow::findPrev);
    connect(ui->btSearchMode, &QPushButton::toggled, this, &MainWindow::toggleSearchMode);
    // System Tray & Window Controls
    connect(actShow, &QAction::triggered, this, &MainWindow::show);
    connect(actHide, &QAction::triggered, this, &MainWindow::hide);
    connect(&tray, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
    // Settings Observer Connections
    connect(&settings, &Settings::ShowHiddenChanged, this, &MainWindow::warningSettingsChanged);
    connect(&settings, &Settings::WindowStateChanged, this, &MainWindow::windowStateChanged);
    connect(&settings, &Settings::ToolbarItemsChanged, this, &MainWindow::actions_changed);
    connect(&settings, &Settings::IconsSizeChanged, this, &MainWindow::icons_size_changed);
    connect(&settings.getScriptModel(), &QAbstractItemModel::dataChanged, this, &MainWindow::cmd_changed);
    connect(&settings, &Settings::NoteFontChanged, this, &MainWindow::applyNoteFontToAll);
}

MainWindow::~MainWindow()
{
    notes->saveAll();
    if (notes->current()) {
        settings.setLastNote(notes->current()->fileName());
    }
    settings.setDialogGeometry(saveGeometry());
    settings.setDialogState(saveState());
    settings.setScripts();
    settings.save();
    delete ui;
}

void MainWindow::CopyNote()
{
    if (notes->empty()) return;
    notes->current()->copy();
}

void MainWindow::NewNote()
{
    if (!note_create_widget)
    {
        note_create_widget = new NoteCreateWidget(notes, this);
        connect(note_create_widget, &NoteCreateWidget::closed, this, &MainWindow::SetCheckedInverse);
        note_create_widget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        note_create_widget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        note_create_widget->setFrameShadow(QFrame::Raised);
    }
    if (ui->mainToolBar->orientation() == Qt::Horizontal)
        note_create_widget->move(ui->mainToolBar->mapToGlobal(QPoint(0, ui->mainToolBar->height())));
    else
        note_create_widget->move(ui->mainToolBar->mapToGlobal(QPoint(ui->mainToolBar->width(), 0)));
    if (actions[itemAdd]->isChecked())
        note_create_widget->show();
    else
        note_create_widget->hide();
}

void MainWindow::NewNotePlain()
{
    notes->create(Note::type_text);
}

void MainWindow::NewNoteHTML()
{
    notes->create(Note::type_html);
}

void MainWindow::NewNoteTODO()
{
    notes->create(Note::type_todo);
}

void MainWindow::NextNote()
{
    if (notes->empty()) return;
    notes->getWidget()->setCurrentIndex(notes->currentIndex() + 1);
}

void MainWindow::PreviousNote()
{
    if (notes->empty()) return;
    notes->getWidget()->setCurrentIndex(notes->currentIndex() - 1);
}

void MainWindow::SetCheckedInverse(bool checked)
{
    actions[itemAdd]->setChecked(!checked);
}

void MainWindow::ToNote(int n)
{
    if (n >= notes->count()) return;
    notes->getWidget()->setCurrentIndex(n);
}

void MainWindow::actions_changed()
{
    ui->mainToolBar->clear();
    const QList<int> items = settings.getToolbarItems();
    // Check if the user has enabled itemFormatHtmlSize in their custom toolbar
    bool htmlSizeEnabled = items.contains(itemFormatHtmlSize);
    if (comboHtmlSize) {
        comboHtmlSize->setVisible(htmlSizeEnabled);
    }
    bool lastWasSeparator = true; // Suppress leading separators
    for (int id : items) {
        if (id == itemSeparator) {
            if (!lastWasSeparator) {
                ui->mainToolBar->addSeparator();
                lastWasSeparator = true;
            }
        } else if (id == itemFormatHtmlSize) {
            if (comboHtmlSize && htmlSizeEnabled) {
                ui->mainToolBar->addWidget(comboHtmlSize);
                lastWasSeparator = false;
            }
        } else if (id >= 0 && id < itemMax && actions[id]) {
            ui->mainToolBar->addAction(actions[id]);
            lastWasSeparator = false;
        }
    }
}

void MainWindow::applyNoteFontToAll()
{
    if (!notes) return;
    QFont newFont = settings.getNoteFont();
    for (int i = 0; i < notes->count(); ++i)
    {
        Note *note = notes->get(i);
        if (!note) continue;
        if (note->type() == Note::type_text || note->type() == Note::type_html)
        {
            if (QWidget *widget = note->widget())
            {
                if (auto *editor = widget->findChild<QTextEdit*>())
                {
                    editor->setFont(newFont);
                }
            }
        }
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::WindowStateChange)
    {
        if (isMinimized() || !isVisible())
        {
            if (autoSaveTimer && autoSaveTimer->isActive())
                autoSaveTimer->stop();
        }
        else if (autoSaveTimer && !autoSaveTimer->isActive())
        {
            autoSaveTimer->start();
        }
    }
    else if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        for (int i = 0; i < itemMax; ++i) {
            if (actions[i]) {
                actions[i]->setText(ToolbarAction(static_cast<item_enum>(i)).text());
            }
        }
        if (actShow) actShow->setText(tr("Show"));
        if (actHide) actHide->setText(tr("Hide"));
        if (notes) {
            notes->retranslate(settings.getLocaleCurrent());
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    purgeOldBackupFiles();
    if (tray.isVisible())
    {
        hide();
        event->ignore();
    }
    if (note_create_widget && note_create_widget->isVisible())
    {
        actions[itemAdd]->setChecked(false);
        note_create_widget->hide();
    }
}

void MainWindow::cmdExec(const QString &command)
{
    if (notes->empty()) return;
    auto *p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QStringList args{"-c", command, "_", notes->current()->absolutePath()};
    connect(p, &QProcess::finished, this, [this, p](int, QProcess::ExitStatus) {
        QByteArray output = p->readAll().trimmed();
        if (!output.isEmpty())
        {
            if (settings.getScriptShowOutput())
                tray.showMessage(tr("Script output"), QString::fromUtf8(output));
            if (settings.getScriptCopyOutput())
                QApplication::clipboard()->setText(QString::fromUtf8(output));
        }
        p->deleteLater();
    });
    p->start("/bin/sh", args);
    if (!p->waitForStarted(4000))
    {
        QMessageBox::warning(this, tr("Script error"), tr("Failed to start:\n%1").arg(command));
        p->deleteLater();
    }
}

void MainWindow::cmd_changed()
{
    cmd_menu.clear();
    ScriptModel &sm = settings.getScriptModel();
    for (int i = 0; i < sm.rowCount(); ++i)
    {
        QAction *act = cmd_menu.addAction(QIcon(sm.getIcon(i)), sm.getName(i));
        QString cmd = sm.getFile(i);
        connect(act, &QAction::triggered, this, [this, cmd]() { cmdExec(cmd); });
    }
    cmd_menu.addSeparator();
    cmd_menu.addAction(tr("Edit command list"), this, &MainWindow::edit_command_list);
}

void MainWindow::commandMenu()
{
    if (!cmd_menu.actions().isEmpty())
    {
        QPoint p = ui->mainToolBar->actionGeometry(actions[itemRun]).center() + pos();
        cmd_menu.exec(p);
    }
    else
    {
        QMessageBox::information(this,
            tr("Commandlist is clear"),
            tr("List of commands is clear!\nYou can add new commands in preferences."));
    }
}

void MainWindow::currentNoteChanged(int old_index, int new_index)
{
    if (old_index != -1)
    {
        Note *note = notes->get(old_index);
        if (note && note->type() == Note::type_html) {
            if (auto *htmlnote = dynamic_cast<HtmlNote*>(note)) {
                disconnect(htmlnote, &HtmlNote::formatChanged, this, &MainWindow::formatChanged);
            }
        }
    }
    bool not_empty = !notes->empty();
    actions[itemPrev]->setEnabled(not_empty && new_index != 0);
    actions[itemNext]->setEnabled(not_empty && new_index != notes->last());
    Note *note = notes->current();
    if (note)
    {
        if (auto *editor = qobject_cast<QTextEdit*>(note->widget())) {
            editor->setReadOnly(m_readOnly);
        }
        switch (note->type())
        {
        case Note::type_html:
        {
            auto *htmlnote = dynamic_cast<HtmlNote*>(note);
            if (htmlnote)
            {
                updateFormatActions(htmlnote->getSelFormat());
                connect(htmlnote, &HtmlNote::formatChanged, this, &MainWindow::formatChanged);
            }
            actions[itemFormatBold]->setEnabled(!m_readOnly);
            actions[itemFormatItalic]->setEnabled(!m_readOnly);
            actions[itemFormatStrikeout]->setEnabled(!m_readOnly);
            actions[itemFormatUnderline]->setEnabled(!m_readOnly);
            actions[itemFormatColor]->setEnabled(!m_readOnly);
            actions[itemFormatSerif]->setEnabled(!m_readOnly);
            if (comboHtmlSize) comboHtmlSize->setEnabled(!m_readOnly);
            break;
        }
        default:
            actions[itemFormatBold]->setEnabled(false);
            actions[itemFormatItalic]->setEnabled(false);
            actions[itemFormatStrikeout]->setEnabled(false);
            actions[itemFormatUnderline]->setEnabled(false);
            actions[itemFormatColor]->setEnabled(false);
            actions[itemFormatSerif]->setEnabled(false);
            if (comboHtmlSize) comboHtmlSize->setEnabled(false);
            break;
        }
    }
    else
    {
        actions[itemFormatBold]->setEnabled(false);
        actions[itemFormatItalic]->setEnabled(false);
        actions[itemFormatStrikeout]->setEnabled(false);
        actions[itemFormatUnderline]->setEnabled(false);
        actions[itemFormatColor]->setEnabled(false);
        actions[itemFormatSerif]->setEnabled(false);
        if (comboHtmlSize) comboHtmlSize->setEnabled(false);
    }
}

// Helper function to resolve the active QTextEdit in zNotes
QTextEdit *MainWindow::currentTextEdit() const
{
    if (notes && notes->current()) {
        Note *note = notes->current();
        if (QWidget *widget = note->widget()) {
            if (auto *editor = widget->findChild<QTextEdit*>()) {
                return editor;
            }
            if (auto *editorDirect = qobject_cast<QTextEdit*>(widget)) {
                return editorDirect;
            }
        }
    }
    return nullptr;
}

void MainWindow::edit_command_list()
{
	configDialog dlg(this);
	dlg.changeTabToCommands();
	dlg.exec();
}

// Perform search/find and restore focus to edSearch so focus never jumps away
void MainWindow::findInCurrentNote(bool next, bool backward)
{
    if (notes->empty()) 
        return;
    Note *currentNote = notes->current();
    if (!currentNote) 
        return;
    QString text = ui->edSearch->text();
    if (text.isEmpty()) {
        highlightAllMatches(false);
        return;
    }
    // Retrieve the active editor from the current note
    QTextEdit *edit = currentNote->findChild<QTextEdit *>();
    if (!edit) 
        return;
    // Construct search flags from UI state
    QTextDocument::FindFlags flags;
    if (ui->tbCase && ui->tbCase->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (backward) {
        flags |= QTextDocument::FindBackward;
    }
    // Perform find on the QTextEdit document
    bool found = edit->find(text, flags);
    // If not found and moving forward, wrap around to the top/bottom
    if (!found && next) {
        QTextCursor cursor = edit->textCursor();
        cursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
        edit->setTextCursor(cursor);
        edit->find(text, flags);
    }
    // Maintain input focus on the search line edit
    ui->edSearch->setFocus(Qt::OtherFocusReason);
    // Refresh extra selection highlights if active
    if (ui->tbHighlightAll && ui->tbHighlightAll->isChecked()) {
        highlightAllMatches(true);
    }
}

void MainWindow::findNext()
{
    QString searchText = ui->edSearch->text();
    if (searchText.isEmpty())
        return;
    QTextDocument::FindFlags flags;
    if (ui->tbCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_searchAllNotes) {
        notes->findNextGlobal(searchText, flags);
    } else {
        notes->findInCurrent(searchText, flags, true);
    }
}

void MainWindow::findPrev()
{
    QString searchText = ui->edSearch->text();
    if (searchText.isEmpty())
        return;
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (ui->tbCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_searchAllNotes) {
        notes->findPrevGlobal(searchText, flags);
    } else {
        notes->findInCurrent(searchText, flags, true);
    }
}

void MainWindow::formatBold()
{
    if (auto *editor = currentTextEdit()) {
        QTextCharFormat fmt;
        bool isBold = editor->fontWeight() > QFont::Weight::Normal;
        fmt.setFontWeight(isBold ? QFont::Weight::Normal : QFont::Weight::Bold);
        editor->mergeCurrentCharFormat(fmt);
    }
}

void MainWindow::formatChanged(const QTextCharFormat &format)
{
    updateFormatActions(format);
}

void MainWindow::formatItalic()
{
    if (auto *editor = currentTextEdit()) {
        QTextCharFormat fmt;
        fmt.setFontItalic(!editor->fontItalic());
        editor->mergeCurrentCharFormat(fmt);
    }
}

void MainWindow::formatSerif()
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (!note) return;
    QWidget *widget = note->widget();
    if (!widget) return;
    QTextEdit *editor = widget->findChild<QTextEdit*>();
    if (!editor) editor = qobject_cast<QTextEdit*>(widget);
    if (editor) {
        QTextCharFormat currentFmt = editor->currentCharFormat();
        QFont font = currentFmt.font();
        bool isSerif = font.styleHint() == QFont::Serif || 
                       font.families().contains(QStringLiteral("serif"), Qt::CaseInsensitive) ||
                       font.family().contains(QStringLiteral("serif"), Qt::CaseInsensitive);
        QTextCharFormat fmt;
        if (isSerif) {
            fmt.setFontFamilies({ QStringLiteral("sans-serif") });
            fmt.setFontStyleHint(QFont::SansSerif);
        } else {
            fmt.setFontFamilies({ QStringLiteral("serif") });
            fmt.setFontStyleHint(QFont::Serif);
        }
        QTextCursor cursor = editor->textCursor();
        if (cursor.hasSelection()) {
            cursor.mergeCharFormat(fmt);
        } else {
            editor->setCurrentCharFormat(fmt);
        }
    }
}

void MainWindow::formatStrikeout()
{
    if (auto *editor = currentTextEdit()) {
        QTextCharFormat fmt;
        fmt.setFontStrikeOut(!editor->currentCharFormat().fontStrikeOut());
        editor->mergeCurrentCharFormat(fmt);
    }
}

void MainWindow::formatTextColor()
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (note->type() != Note::type_html) return;
    if (auto *editor = currentTextEdit()) {
        QColor currentColor = editor->textColor();
        QColor selectedColor = QColorDialog::getColor(currentColor, this, "Select Text Color");
        if (selectedColor.isValid()) {
            QTextCharFormat fmt;
            fmt.setForeground(QBrush(selectedColor));
            editor->mergeCurrentCharFormat(fmt);
        }
    }
}

void MainWindow::formatUnderline()
{
    if (auto *editor = currentTextEdit()) {
        QTextCharFormat fmt;
        fmt.setFontUnderline(!editor->fontUnderline());
        editor->mergeCurrentCharFormat(fmt);
    }
}

void MainWindow::formatZoomIn()
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (!note) return;
    if (QWidget *widget = note->widget()) {
        QTextEdit *editor = widget->findChild<QTextEdit*>();
        if (!editor) editor = qobject_cast<QTextEdit*>(widget);
        if (editor) {
            // First try standard zoom
            editor->zoomIn(1);
            // For HTML documents with hardcoded font sizes, also scale the default font
            QFont f = editor->font();
            f.setPointSize(f.pointSize() + 1);
            editor->setFont(f);
        }
    }
}

void MainWindow::formatZoomOut()
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (!note) return;
    if (QWidget *widget = note->widget()) {
        QTextEdit *editor = widget->findChild<QTextEdit*>();
        if (!editor) editor = qobject_cast<QTextEdit*>(widget);
        if (editor) {
            editor->zoomOut(1);
            QFont f = editor->font();
            if (f.pointSize() > 4) {
                f.setPointSize(f.pointSize() - 1);
                editor->setFont(f);
            }
        }
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
	QMainWindow::hideEvent(event);
	settings.setDialogGeometry(saveGeometry());
	actShow->setEnabled(true);
	actHide->setDisabled(true);
	if (notes->current()) { notes->saveAll(); }
	if (note_create_widget && note_create_widget->isVisible())
	{
		actions[itemAdd]->setChecked(false);
		note_create_widget->hide();
	}
}

void MainWindow::highlightAllMatches(bool enable)
{
    if (!notes || notes->empty())
        return;
    Note *currentNote = notes->current();
    if (!currentNote)
        return;
    if (enable && !ui->edSearch->text().isEmpty()) {
        QTextDocument::FindFlags flags;
        if (ui->tbCase && ui->tbCase->isChecked()) {
            flags |= QTextDocument::FindCaseSensitively;
        }
        // Delegate highlighting directly to the note abstraction
        currentNote->highlightAll(ui->edSearch->text(), flags);
    } else {
        // Clear highlights by passing an empty search term
        currentNote->highlightAll(QString(), {});
    }
}

void MainWindow::icons_size_changed()
{
	QSize size(settings.getIconsSize(), settings.getIconsSize());
	ui->mainToolBar->setIconSize(size);
}

bool MainWindow::isNoteSupportsPrinting(const Note *note) const
{
    if (!note)
        return false;
    if (!note->isDocumentSupported()) {
        QMessageBox::warning(const_cast<MainWindow*>(this),
            tr("Note printing"),
            tr("There is no printing support for current note"));
        return false;
    }
    return true;
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    if (note_create_widget && note_create_widget->isVisible())
    {
        if (ui->mainToolBar->orientation() == Qt::Horizontal)
            note_create_widget->move(ui->mainToolBar->mapToGlobal(QPoint(0, ui->mainToolBar->height())));
        else
            note_create_widget->move(ui->mainToolBar->mapToGlobal(QPoint(ui->mainToolBar->width(), 0)));
    }
}

void MainWindow::notePrint()
{
    Note *currentNote = notes->current();
    if (!isNoteSupportsPrinting(currentNote)) return;
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle(tr("Print note"));
    if (dlg.exec() == QDialog::Accepted) {
        if (!currentNote->document()) {
            QMessageBox::warning(this, tr("Note printing"), tr("Access error to note's text"));
            return;
        }
        currentNote->document()->print(&printer);
    }
#endif
}

void MainWindow::notePrintPreview()
{
    Note *currentNote = notes->current();
    if (!isNoteSupportsPrinting(currentNote)) return;
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &MainWindow::printPreview);
    preview.exec();
#endif
}

void MainWindow::notePrintPdf()
{
    Note *currentNote = notes->current();
    if (!isNoteSupportsPrinting(currentNote)) return;
    QString defaultFileName = QDir::currentPath() + "/" + currentNote->title();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export PDF"),
        defaultFileName, QString("*.pdf"));
    if (!fileName.isEmpty()) {
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName.append(".pdf");
#ifndef QT_NO_PRINTER
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        if (!currentNote->document()) {
            QMessageBox::warning(this, tr("Note printing"), tr("Access error to note's text"));
            return;
        }
        currentNote->document()->print(&printer);
#endif
    }
}

void MainWindow::on_actionFontSerif_triggered(bool checked)
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (!note) return;
    QWidget *widget = note->widget();
    if (!widget) return;
    QTextEdit *editor = widget->findChild<QTextEdit*>();
    if (!editor) editor = qobject_cast<QTextEdit*>(widget);
    if (!editor) return;
    QTextCharFormat fmt;
    if (checked) {
        fmt.setFontFamilies({QStringLiteral("Serif"), QStringLiteral("Times New Roman")});
        fmt.setFontStyleHint(QFont::Serif);
    } else {
        fmt.setFontFamilies({QStringLiteral("Sans"), QStringLiteral("Arial"), QStringLiteral("Helvetica")});
        fmt.setFontStyleHint(QFont::SansSerif);
    }
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection()) {
        cursor.mergeCharFormat(fmt);
    } else {
        editor->setCurrentCharFormat(fmt);
    }
    editor->setFocus();
    // Synchronize button visual state immediately
    updateFormatActions(editor->textCursor().charFormat());
}

// HTML Font Size (1 to 6): 1=x-small, 2=small, 3=medium, 4=large, 5=x-large, 6=xx-large
void MainWindow::on_actionHtmlSize_activated(int index)
{
    if (notes->empty()) return;
    Note *note = notes->current();
    if (!note) return;
    QWidget *widget = note->widget();
    if (!widget) return;
    QTextEdit *editor = widget->findChild<QTextEdit*>();
    if (!editor) editor = qobject_cast<QTextEdit*>(widget);
    if (editor && index >= 0 && index < 6) {
        QTextCursor cursor = editor->textCursor();
        if (index == 2) {
            // "Default": Remove ALL font-size attributes from selection
            if (cursor.hasSelection()) {
                int start = cursor.selectionStart();
                int end = cursor.selectionEnd();
                cursor.setPosition(start);
                cursor.setPosition(end, QTextCursor::KeepAnchor);
                // Re-apply format with font size properties stripped
                QTextCharFormat fmt = cursor.charFormat();
                fmt.clearProperty(QTextFormat::FontPointSize);
                fmt.clearProperty(QTextFormat::FontSizeAdjustment);
                cursor.setCharFormat(fmt);
            } else {
                QTextCharFormat cur = editor->currentCharFormat();
                cur.clearProperty(QTextFormat::FontPointSize);
                cur.clearProperty(QTextFormat::FontSizeAdjustment);
                editor->setCurrentCharFormat(cur);
            }
        } else {
            // Non-default sizes (x-small, small, large, x-large, xx-large)
            int sizeAdjustment = index - 2; // Maps 0..5 to -2..+3
            QTextCharFormat fmt;
            fmt.clearProperty(QTextFormat::FontPointSize);
            fmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
            if (cursor.hasSelection()) {
                cursor.mergeCharFormat(fmt);
            } else {
                editor->setCurrentCharFormat(fmt);
            }
        }
        editor->setFocus();
        updateFormatActions(editor->textCursor().charFormat());
    }
}

// Auto-connected UI Slots:
void MainWindow::on_btSearchClose_clicked()
{
    if (actions[itemSearch]) {
        actions[itemSearch]->setChecked(false);
    }
}

void MainWindow::on_edSearch_returnPressed()
{
    findInCurrentNote(true, false); // Find Next
}

void MainWindow::on_edSearch_textChanged(const QString &text)
{
    Q_UNUSED(text);
    findInCurrentNote(false);
}

void MainWindow::on_tbCase_toggled(bool checked)
{
    Q_UNUSED(checked);
    findInCurrentNote(false);
}

void MainWindow::on_tbHighlightAll_toggled(bool checked)
{
    highlightAllMatches(checked);
}

void MainWindow::performSearch()
{
    QString searchText = ui->edSearch->text();
    if (searchText.isEmpty())
        return;
    QTextDocument::FindFlags flags;
    if (ui->tbCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_searchAllNotes) {
        // Global search across all notes in NoteList
        int matches = notes->findAll(searchText, flags);
        // Update status bar with match count
        statusBar()->showMessage(tr("Found in %1 note(s)").arg(matches), 3000);
    } else {
        // Search current active note
        notes->findInCurrent(searchText, flags);
    }
    if (ui->tbHighlightAll->isChecked()) {
        if (m_searchAllNotes) {
            // Implement global highlight if available, or current note highlight
            notes->highlightInCurrent(searchText, flags);
        } else {
            notes->highlightInCurrent(searchText, flags);
        }
    }
}

void MainWindow::printPreview(QPrinter *printer)
{
    Note *currentNote = notes->current();
    if (!isNoteSupportsPrinting(currentNote)) return;
#ifndef QT_NO_PRINTER
    if (!currentNote->document()) {
        QMessageBox::warning(this, tr("Note printing"), tr("Access error to note's text"));
        return;
    }
    currentNote->document()->print(printer);
#endif
}

void MainWindow::purgeOldBackupFiles()
{
    QString notesPath = settings.getNotesPath();
    if (notesPath.isEmpty() || !QDir(notesPath).exists()) return;
    QDir dir(notesPath);
    QStringList filters;
    filters << QStringLiteral("*.bk");
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList fileList = dir.entryInfoList();
    QDateTime sevenDaysAgo = QDateTime::currentDateTime().addDays(-7);
    for (const QFileInfo &fileInfo : fileList) {
        if (fileInfo.lastModified() < sevenDaysAgo) {
            QFile::remove(fileInfo.absoluteFilePath());
        }
    }
    // Clear the name filters so the directory object isn't left filtering 
    // exclusively for .bk files elsewhere in the application lifecycle.
    dir.setNameFilters(QStringList());
}

void MainWindow::saveAllNotes()
{
    if (notes && !notes->empty()) {
        notes->saveAll();
    }
    settings.save();
}

void MainWindow::showAboutDialog()
{
    aboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    restoreGeometry(settings.getDialogGeometry());
    actShow->setEnabled(false);
    actHide->setDisabled(false);
}

void MainWindow::showPrefDialog()
{
    configDialog dlg(this);
    dlg.exec();
}

void MainWindow::showSearchBar(bool show)
{
    ui->wSearch->setVisible(show);
    // Keep the toolbar action checkmark state in sync without re-triggering slots
    if (actions[itemSearch] && actions[itemSearch]->isChecked() != show) {
        actions[itemSearch]->blockSignals(true);
        actions[itemSearch]->setChecked(show);
        actions[itemSearch]->blockSignals(false);
    }
    if (show) {
        ui->edSearch->setFocus();
        ui->edSearch->selectAll();
    } else {
        ui->edSearch->clear();
        // Return focus to the note editor when search bar closes
        if (!notes->empty() && notes->current() && notes->current()->widget()) {
            notes->current()->widget()->setFocus();
        }
    }
}

void MainWindow::toggleReadOnly()
{
    m_readOnly = !m_readOnly;
    // Apply read-only mode to all note editors
    for (int i = 0; i < notes->count(); ++i) {
        if (Note *note = notes->get(i)) {
            if (QWidget *w = note->widget()) {
                // Try direct cast first (when note->widget() IS the QTextEdit)
                QTextEdit *editor = qobject_cast<QTextEdit*>(w);
                if (!editor) {
                    // Fall back to searching child widgets
                    editor = w->findChild<QTextEdit*>();
                }
                if (editor) {
                    editor->setReadOnly(m_readOnly);
                }
            }
        }
    }
    // Update the action state, icon, and tooltip
    if (actions[itemReadOnly]) {
        actions[itemReadOnly]->setChecked(m_readOnly);
        actions[itemReadOnly]->setIcon(QIcon(m_readOnly ? ":/res/toolbar/padlocked.png" 
                                                        : ":/res/toolbar/padlock.png"));
        actions[itemReadOnly]->setToolTip(m_readOnly ? tr("Unlock Editing (Ctrl+L)") 
                                                        : tr("Lock Note / View Only (Ctrl+L)"));
    }
    // Refresh active note state to immediately update toolbar button enabled states
    currentNoteChanged(-1, notes->currentIndex());
    statusBar()->showMessage(m_readOnly ? tr("Mode: View Only (Locked)") 
                                        : tr("Mode: Editing Enabled"), 2000);
}

void MainWindow::toggleSearchMode(bool searchAll)
{
    m_searchAllNotes = searchAll;
    if (m_searchAllNotes) {
        ui->label->setText(tr("Search for:"));
        ui->btSearchMode->setText(tr("All"));
        ui->btSearchMode->setToolTip(tr("Searching across all notes (Click for current page only)"));
    } else {
        ui->label->setText(tr("Find:"));
        ui->btSearchMode->setText(tr("Page"));
        ui->btSearchMode->setToolTip(tr("Searching current note only (Click for all notes)"));
        statusBar()->clearMessage(); // Hide/clear message when returning to Page mode
    }
    if (!ui->edSearch->text().isEmpty()) {
        performSearch();
    }
}
void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (isHidden())
            show();
        else
            hide();
    }
}









void MainWindow::updateFormatActions(const QTextCharFormat &fmt)
{
    QFont font = fmt.font();
    actions[itemFormatBold]->setChecked(font.bold());
    actions[itemFormatItalic]->setChecked(font.italic());
    actions[itemFormatStrikeout]->setChecked(font.strikeOut());
    actions[itemFormatUnderline]->setChecked(font.underline());
    bool isSerif = font.styleHint() == QFont::Serif ||
                   font.families().contains(QStringLiteral("serif"), Qt::CaseInsensitive) ||
                   font.family().contains(QStringLiteral("serif"), Qt::CaseInsensitive);
    actions[itemFormatSerif]->setChecked(isSerif);
    if (comboHtmlSize) {
        int targetIndex = 2; // Default (medium)
        // Inspect cursor format directly from current editor if possible
        QTextCharFormat activeFmt = fmt;
        if (Note *note = notes->current()) {
            if (auto *editor = note->widget()->findChild<QTextEdit*>()) {
                activeFmt = editor->textCursor().charFormat();
            }
        }
        if (activeFmt.hasProperty(QTextFormat::FontSizeAdjustment)) {
            int adj = activeFmt.intProperty(QTextFormat::FontSizeAdjustment);
            targetIndex = adj + 2;
        } else if (activeFmt.fontPointSize() > 0) {
            qreal baseSize = 11.0;
            if (Note *note = notes->current()) {
                if (auto *editor = note->widget()->findChild<QTextEdit*>()) {
                    baseSize = editor->document()->defaultFont().pointSizeF();
                    if (baseSize <= 0) baseSize = 11.0;
                }
            }
            qreal ratio = activeFmt.fontPointSize() / baseSize;
            if (ratio <= 0.70)      targetIndex = 0;
            else if (ratio <= 0.90) targetIndex = 1;
            else if (ratio <= 1.10) targetIndex = 2;
            else if (ratio <= 1.35) targetIndex = 3;
            else if (ratio <= 1.75) targetIndex = 4;
            else                    targetIndex = 5;
        }
        if (targetIndex < 0) targetIndex = 0;
        if (targetIndex > 5) targetIndex = 5;
        comboHtmlSize->blockSignals(true);
        comboHtmlSize->setCurrentIndex(targetIndex);
        comboHtmlSize->blockSignals(false);
    }
    if (actions[itemFormatSerif]) {
        QTextCharFormat activeFmt = fmt;
        if (Note *note = notes->current()) {
            if (auto *editor = note->widget()->findChild<QTextEdit*>()) {
                activeFmt = editor->textCursor().charFormat();
            }
        }
        QFont font = activeFmt.font();
        QString family = font.family().toLower();
        QStringList families = font.families();
        bool isSerif = (font.styleHint() == QFont::Serif) ||
                       family.contains(QStringLiteral("serif")) ||
                       family.contains(QStringLiteral("times")) ||
                       family.contains(QStringLiteral("georgia"));
        for (const QString &fam : families) {
            if (fam.toLower().contains(QStringLiteral("serif")) || 
                fam.toLower().contains(QStringLiteral("times"))) {
                isSerif = true;
                break;
            }
        }
        actions[itemFormatSerif]->blockSignals(true);
        actions[itemFormatSerif]->setChecked(isSerif);
        actions[itemFormatSerif]->blockSignals(false);
    }
}

void MainWindow::warningSettingsChanged()
{
    QMessageBox::information(this, tr("Settings changed"),
        tr("You need restart application to get effect."));
}

void MainWindow::windowStateChanged()
{
    Qt::WindowFlags flags = Qt::Window;
    if (settings.getHideFrame()) flags |= Qt::FramelessWindowHint;
    if (settings.getStayTop()) flags |= Qt::WindowStaysOnTopHint;
    bool window_is_visible = isVisible();
    setWindowFlags(flags);
    if (window_is_visible) show();
}

