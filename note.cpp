#include "note.h"
#include "settings.h"

#include <QDir>

/*
  Note is an abstract base class for zNotes documents
*/

Note::Note(const QFileInfo& fileinfo, Note::Type type_new, QObject *parent)
    : QObject(parent),
      _type(type_new),
      file_info(fileinfo),
      content_changed(false)
{
    updateTitle(settings.getShowExtensions());
}

Note::~Note()
{
    if (file.isOpen())
    {
        file.close();
    }
    // CRITICAL: Do NOT call file.remove() in destructor.
}

// Updating note title based on preference settings
void Note::updateTitle(bool show_extensions)
{
    file.setFileName(file_info.absoluteFilePath());
    
    const QString filename = file_info.fileName();
    if (show_extensions || filename.startsWith('.'))
    {
        _title = filename;
    }
    else
    {
        _title = file_info.completeBaseName();
    }
}

// Renaming note
void Note::rename(const QString &new_name)
{
    if (file.isOpen())
        file.close();

    QString absolute_file_path = file_info.dir().absoluteFilePath(new_name);
    if (file.rename(absolute_file_path))
    {
        file_info.setFile(file);
        updateTitle(settings.getShowExtensions());
    }
}

// Moving note to another folder
void Note::move(const QString &new_dirname)
{
    if (file.isOpen())
        file.close();

    QString filename = file_info.fileName();
    QString absolute_file_path = QDir(new_dirname).absoluteFilePath(filename);
    if (file.rename(absolute_file_path))
    {
        file_info.setFile(file);
    }
}

bool Note::find(const QString &text, QTextDocument::FindFlags flags, bool from_start)
{
    Q_UNUSED(text)
    Q_UNUSED(flags)
    Q_UNUSED(from_start)
    return false;
}

void Note::highlightAll(const QString &text, QTextDocument::FindFlags flags)
{
    Q_UNUSED(text)
    Q_UNUSED(flags)
}

// Slot called when note content changes
void Note::contentChanged()
{
    content_changed = true;
}
