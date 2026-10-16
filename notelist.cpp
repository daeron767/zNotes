#include "notelist.h"
#include "settings.h"

#include "textnote.h"
#include "htmlnote.h"
#include "picturenote.h"
#ifdef NOTE_TODO_FORMAT
    #include "todonote.h"
#endif
#ifdef NOTE_XML_FORMAT
    #include "note_xml.h"
#endif

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextCursor>
#include <QTextEdit>

NoteList::NoteList(QWidget *parent)
    : QObject(parent),
      FILE_WITH_CURSOR_POSITIONS(".znotes_cursor_positions"),
      parentWidget(parent)
{
    initNoteTypes();
    tabs = new ZTabWidget(parent);
    tabs->setDocumentMode(true);
    tabs->setTabPosition(static_cast<QTabWidget::TabPosition>(settings.getTabPosition()));
    // Directory configuration
    dir.setPath(settings.getNotesPath());
    if (!dir.exists())
    {
        if (!dir.mkpath(dir.path()))
            dir.setPath("");
    }
    while (dir.path().isEmpty())
    {
        QString selected = QFileDialog::getExistingDirectory(
            nullptr,
            tr("Select place for notes directory"),
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!selected.isEmpty())
        {
            QString newPath = QDir(selected + '/' + tr("Notes")).absolutePath();
            settings.setNotesPath(newPath);
            dir.setPath(newPath);
            if (!dir.exists())
            {
                if (!dir.mkpath(dir.path()))
                    dir.setPath("");
            }
            if (!dir.isReadable())
                dir.setPath("");
        }
    }
    // Cursor positions
    QFile cursorPositionFile(dir.filePath(FILE_WITH_CURSOR_POSITIONS));
    QMap<QString, int> noteCursorPositions;
    if (cursorPositionFile.exists())
    {
        QSettings cursorSettings(cursorPositionFile.fileName(), QSettings::IniFormat);
        cursorSettings.beginGroup("Positions");
        const QStringList keys = cursorSettings.childKeys();
        for (const QString &key : keys)
        {
            noteCursorPositions[key] = cursorSettings.value(key, 0).toInt();
        }
        cursorSettings.endGroup();
    }
    // Load initial files
    dir.setFilter(settings.getShowHidden() ? (QDir::Files | QDir::Hidden | QDir::Readable)
                                           : (QDir::Files | QDir::Readable));
    QFileInfoList flist = dir.entryInfoList();
    const QString &last_note_title = settings.getLastNote();
    int old_index = -1;
    for (int i = 0; i < flist.size(); ++i)
    {
        QString fileName = flist.at(i).fileName();
        if (fileName == FILE_WITH_CURSOR_POSITIONS || 
            fileName.endsWith(QStringLiteral(".bk"), Qt::CaseInsensitive))
        {
            continue;
        }
        Note *note = this->add(flist.at(i), false);
        if (old_index == -1 && note->fileName() == last_note_title)
        {
            old_index = i;
        }
        if (note->widget())
        {
            if (auto *editor = qobject_cast<QTextEdit*>(note->widget()))
            {
                if (noteCursorPositions.contains(note->fileName()))
                {
                    int pos = noteCursorPositions[note->fileName()];
                    QTextCursor cursor = editor->textCursor();
                    cursor.setPosition(pos);
                    editor->setTextCursor(cursor);
                    editor->ensureCursorVisible();
                }
            }
        }
    }
    if (old_index != -1)
        tabs->setCurrentIndex(old_index);
    else if (empty())
        create(Note::type_html);
    current_index = tabs->currentIndex();
    if (tabs->currentWidget())
    {
        tabs->currentWidget()->setFocus();
    }
    watcher = new QFileSystemWatcher(this);
    watcher->addPath(dir.absolutePath());
    // PMF Signal/Slot connections
    connect(tabs, &QTabWidget::currentChanged, this, &NoteList::currentTabChanged);
    connect(&settings, &Settings::ShowExtensionsChanged, this, &NoteList::showExtensionsChanged);
    connect(&settings, &Settings::TabPositionChanged, this, &NoteList::tabPositionChanged);
    connect(&settings, &Settings::NotesPathChanged, this, &NoteList::notesPathChanged);
    connect(&settings, &Settings::ShowHiddenChanged, this, &NoteList::scanForNewFiles);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &NoteList::fileModifiedExternally);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &NoteList::scanForNewFiles);
    connect(tabs, &ZTabWidget::tabBarDoubleClickedOnTab, this, &NoteList::changeNoteName);
}

NoteList::~NoteList()
{
    tabs->clear();
    qDeleteAll(vec);
    vec.clear();
}

void NoteList::initNoteTypes()
{
    registerType(Note::type_text, QObject::tr("Text Note"),
                 QObject::tr("Simple text Note"), ":/res/note_types/txt.png",
                 ":/res/note_types/16/txt.png", "txt,");
    registerType(Note::type_html, QObject::tr("HTML Note"),
                 QObject::tr("Simple Note with text formatting"), ":/res/note_types/html.png",
                 ":/res/note_types/16/html.png", "html,htm");
    registerType(Note::type_picture, QObject::tr("Picture Note"), QObject::tr("Simple text Note"),
                 ":/res/note_types/pic.png", ":/res/note_types/16/pic.png", "png,jpeg,jpg,gif",
                 false);
#ifdef NOTE_TODO_FORMAT
    registerType(Note::type_todo, QObject::tr("TODO Note"), QObject::tr("Simple TODO list"),
                 ":/res/note_types/todo.png", ":/res/note_types/16/todo.png", "ztodo");
#endif
#ifdef NOTE_XML_FORMAT
    registerType(Note::type_xml, QObject::tr("XML Note"), QObject::tr("XML file"),
                 ":/res/note_types/xml.png", ":/res/note_types/16/xml.png", "xml");
#endif
}

void NoteList::registerType(Note::Type id,
                             const QString &title,
                             const QString &description,
                             const QString &big_icon_path,
                             const QString &small_icon_path,
                             const QString &extensions,
                             bool visible)
{
    note_types[id] = NoteType(id, title, description, big_icon_path, small_icon_path, extensions, visible);
    for (const QString &ext : note_types[id].extensions())
    {
        NOTE_TYPE_MAP[ext] = id;
    }
}

void NoteList::scanForNewFiles()
{
    dir.setFilter(settings.getShowHidden() ? (QDir::Files | QDir::Hidden | QDir::Readable)
                                           : (QDir::Files | QDir::Readable));
    dir.refresh();
    const QFileInfoList flist = dir.entryInfoList();
    for (const QFileInfo &fi : flist)
    {
        QString fileName = fi.fileName();
        if (fileName == FILE_WITH_CURSOR_POSITIONS ||
           fileName.endsWith(QStringLiteral(".bk"), Qt::CaseInsensitive))
        {
            continue;
        }
        if (!notes_filenames.contains(fi.fileName()))
        {
            add(fi);
        }
    }
}

void NoteList::changeNoteName(int tabIndex)
{
    Q_UNUSED(tabIndex)
    renameCurrentNote();
}

Note::Type NoteList::getType(const QFileInfo &fileinfo) const
{
    return NOTE_TYPE_MAP.value(fileinfo.suffix().toLower(), Note::type_text);
}

void NoteList::create(Note::Type type, const QString &name)
{
    const QString extension = noteTypes()[type].defaultExtension();
    QString filename = QString("%1.%2").arg(name.isEmpty() ? "0" : name, extension);
    QFile file(dir.filePath(filename));
    int n = 0;
    while (file.exists())
    {
        filename = QString("%1.%2").arg(++n).arg(extension);
        file.setFileName(dir.filePath(filename));
    }
    add(QFileInfo(file));
}

Note* NoteList::add(const QFileInfo &fileinfo, bool set_current)
{
    Note *note = nullptr;
    Note::Type type = getType(fileinfo);
    switch (type)
    {
        case Note::type_text:
            note = new TextNote(fileinfo, type, this);
            break;
        case Note::type_html:
            note = new HtmlNote(fileinfo, type, this);
            break;
        case Note::type_picture:
            note = new PictureNote(fileinfo, type, this);
            break;
#ifdef NOTE_TODO_FORMAT
        case Note::type_todo:
            note = new TodoNote(fileinfo, type, this);
            break;
#endif
#ifdef NOTE_XML_FORMAT
        case Note::type_xml:
            note = new XmlNote(fileinfo, type, this);
            break;
#endif
        default:
            note = new TextNote(fileinfo, type, this);
            break;
    }
    vec.append(note);
    tabs->addTab(note->widget(), note->title());
    notes_filenames.insert(fileinfo.fileName());
    if (set_current)
    {
        tabs->setCurrentWidget(note->widget());
    }
    return note;
}

void NoteList::remove(int i)
{
    if (i < 0 || i >= vec.size())
        return;
    disconnect(tabs, &QTabWidget::currentChanged, this, &NoteList::currentTabChanged);
    Note *note = vec.takeAt(i);
    tabs->removeTab(i);
    if (note) {
        QString filename = note->fileName();
        // Construct full path using settings notes path if filename is relative
        QFileInfo info(filename);
        if (info.isRelative()) {
            info.setFile(settings.getNotesPath(), filename);
        }
        QString filePath = info.absoluteFilePath();
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            QString backupPath = filePath + QStringLiteral(".bk");
            // Overwrite any existing older backup file of the same name
            if (QFile::exists(backupPath)) {
                QFile::remove(backupPath);
            }
            // Soft-delete: rename to .bk
            if (QFile::rename(filePath, backupPath)) {
                // Touch file to update modification timestamp for retention purging
                QFile backupFile(backupPath);
                if (backupFile.open(QIODevice::ReadWrite)) {
                    backupFile.close();
                }
            }
        }
        note->deleteLater();
        notes_filenames.remove(filename);
    }
    current_index = tabs->currentIndex();
    connect(tabs, &QTabWidget::currentChanged, this, &NoteList::currentTabChanged);
    emit currentNoteChanged(-1, current_index);
}

void NoteList::move(const QString &newPath, const QString &oldPath)
{
    for (Note *note : vec)
    {
        note->move(newPath);
    }
    QFile cursorPositionFile(QDir(oldPath).filePath(FILE_WITH_CURSOR_POSITIONS));
    if (cursorPositionFile.exists())
    {
        cursorPositionFile.remove();
    }
}

void NoteList::fileModifiedExternally(const QString &path)
{
    // Reload the modified note buffer if it matches an open note
    for (int i = 0; i < count(); ++i) {
        if (Note *note = get(i)) {
            if (QFileInfo(note->absolutePath()) == QFileInfo(path)) {
                note->load(); // Reloads content from disk
                break;
            }
        }
    }
}

bool NoteList::find(const QString &text, QTextDocument::FindFlags flags, bool wrap)
{
    Note *note = current();
    if (!note)
        return false;
    // Delegate directly to the current note's unified find function
    return note->find(text, flags, wrap);
}

int NoteList::findAll(const QString &text, QTextDocument::FindFlags flags)
{
    int matchCount = 0;
    Qt::CaseSensitivity cs = (flags & QTextDocument::FindCaseSensitively) 
                             ? Qt::CaseSensitive 
                             : Qt::CaseInsensitive;
    for (Note *note : vec) {
        if (!note || !note->widget()) 
            continue;
        // Retrieve text editor widget from note to check plaintext content
        if (auto *editor = qobject_cast<QTextEdit*>(note->widget())) {
            if (editor->toPlainText().contains(text, cs)) {
                matchCount++;
            }
        }
    }
    return matchCount;
}

bool NoteList::findInCurrent(const QString &text, QTextDocument::FindFlags flags, bool wrap)
{
    if (Note *currentNote = current()) {
        bool found = currentNote->find(text, flags, wrap);
        if (found && currentNote->widget()) {
            currentNote->widget()->setFocus();
        }
        return found;
    }
    return false;
}

bool NoteList::findNextGlobal(const QString &text, QTextDocument::FindFlags flags)
{
    if (text.isEmpty() || vec.isEmpty())
        return false;

    // First try searching forward in the current note
    if (Note *cur = current()) {
        if (cur->find(text, flags, false)) {
            if (cur->widget()) cur->widget()->setFocus();
            return true;
        }
    }
    // If current note has no more matches, cycle through remaining notes
    int startIdx = currentIndex();
    int count = vec.size();
    for (int i = 1; i < count; ++i) {
        int targetIdx = (startIdx + i) % count;
        Note *note = vec.at(targetIdx);
        if (!note || !note->widget())
            continue;
        // Reset cursor to start of note for forward search
        if (auto *editor = qobject_cast<QTextEdit*>(note->widget())) {
            QTextCursor cursor = editor->textCursor();
            cursor.movePosition(QTextCursor::Start);
            editor->setTextCursor(cursor);
        }
        if (note->find(text, flags, false)) {
            setCurrent(targetIdx); // Switch to matching tab
            note->widget()->setFocus();
            return true;
        }
    }
    return false;
}

bool NoteList::findPrevGlobal(const QString &text, QTextDocument::FindFlags flags)
{
    if (text.isEmpty() || vec.isEmpty())
        return false;
    // First try searching backward in current note
    if (Note *cur = current()) {
        if (cur->find(text, flags, false)) {
            if (cur->widget()) cur->widget()->setFocus();
            return true;
        }
    }

    // If no previous matches in current note, cycle backward through other notes
    int startIdx = currentIndex();
    int count = vec.size();
    for (int i = 1; i < count; ++i) {
        int targetIdx = (startIdx - i + count) % count;
        Note *note = vec.at(targetIdx);
        if (!note || !note->widget())
            continue;
        // Reset cursor to end of note for backward search
        if (auto *editor = qobject_cast<QTextEdit*>(note->widget())) {
            QTextCursor cursor = editor->textCursor();
            cursor.movePosition(QTextCursor::End);
            editor->setTextCursor(cursor);
        }
        if (note->find(text, flags, false)) {
            setCurrent(targetIdx); // Switch to matching tab
            note->widget()->setFocus();
            return true;
        }
    }
    return false;
}

void NoteList::highlightInCurrent(const QString &text, QTextDocument::FindFlags flags)
{
    if (Note *currentNote = current()) {
        currentNote->highlightAll(text, flags);
    }
}

void NoteList::removeCurrentNote()
{
    Note *note = current();
    if (!note)
        return;
    QMessageBox msgBox(QMessageBox::Question,
                       tr("Delete Note"),
                       tr("Do you really want to delete note %1?").arg(note->title()),
                       QMessageBox::Yes | QMessageBox::No,
                       parentWidget);
    if (msgBox.exec() == QMessageBox::Yes)
    {
        remove(currentIndex());
    }
}

void NoteList::renameCurrentNote()
{
    if (empty())
        return;
    Note *note = current();
    if (!note)
        return;
    bool ok = false;
    const QString fname = note->fileName();
    int typedot = fname.lastIndexOf('.');
    QString filename = fname;
    QString extension;
    if (typedot != -1)
    {
        extension = fname.mid(typedot);
        filename.truncate(typedot);
    }
    QString new_name = QInputDialog::getText(parentWidget,
                                             tr("Rename note"),
                                             tr("New name:"),
                                             QLineEdit::Normal,
                                             filename,
                                             &ok);
    if (ok && !new_name.isEmpty())
    {
        new_name.append(extension);
        QFile file(dir.filePath(new_name));
        if (!file.exists())
        {
            rename(current_index, new_name);
        }
        else
        {
            QMessageBox::information(parentWidget,
                                     tr("Note renaming"),
                                     tr("Note %1 already exists!").arg(new_name));
        }
    }
}

void NoteList::rename(int index, const QString &title)
{
    if (index < 0 || index >= vec.size())
        return;
    Note *note = vec.at(index);
    note->save();
    notes_filenames.remove(note->fileName());
    note->rename(title);
    tabs->setTabText(index, note->title());
    notes_filenames.insert(note->fileName());
}

void NoteList::currentTabChanged(int index)
{
    if (index == -1)
        return;
    if (current_index != -1 && current())
    {
        current()->save();
        if (tabs->currentWidget())
        {
            tabs->currentWidget()->setFocus();
        }
    }
    int old_index = current_index;
    current_index = index;
    const QString path = vec[current_index]->absolutePath();
    emit currentNoteChanged(old_index, current_index);
}

void NoteList::showExtensionsChanged(bool show_extensions)
{
    for (int i = 0; i < vec.size(); ++i)
    {
        vec[i]->updateTitle(show_extensions);
        tabs->setTabText(i, vec[i]->title());
    }
}

void NoteList::tabPositionChanged()
{
    tabs->setTabPosition(static_cast<QTabWidget::TabPosition>(settings.getTabPosition()));
}

void NoteList::saveAll()
{
    QMap<QString, int> noteCursorPositions;
    for (Note *note : vec)
    {
        note->save(true);
        if (note->widget())
        {
            if (auto *editor = qobject_cast<QTextEdit*>(note->widget()))
            {
                noteCursorPositions[note->fileName()] = editor->textCursor().position();
            }
        }
    }
    QFile cursorPositionFile(dir.filePath(FILE_WITH_CURSOR_POSITIONS));
    if (cursorPositionFile.exists())
    {
        cursorPositionFile.remove();
    }
    QSettings cursorSettings(cursorPositionFile.fileName(), QSettings::IniFormat);
    cursorSettings.beginGroup("Positions");
    for (auto it = noteCursorPositions.constBegin(); it != noteCursorPositions.constEnd(); ++it)
    {
        cursorSettings.setValue(it.key(), it.value());
    }
    cursorSettings.endGroup();
}

void NoteList::notesPathChanged()
{
    QMessageBox msgBox(QMessageBox::Question,
                       tr("Move notes"),
                       tr("Notes path changed!\nDo you want to move your notes to new place?"),
                       QMessageBox::Yes | QMessageBox::No,
                       parentWidget);
    if (msgBox.exec() == QMessageBox::Yes)
    {
        QString previousDirPath = dir.absolutePath();
        dir.setPath(settings.getNotesPath());
        move(dir.absolutePath(), previousDirPath);
    }
    else
    {
        QMessageBox::information(parentWidget,
                                 tr("Notes path change"),
                                 tr("You need to restart the application for changes to take effect."));
    }
}

void NoteList::retranslate(const QLocale &locale)
{
    for (Note *note : vec)
    {
        note->retranslate(locale);
    }
}
