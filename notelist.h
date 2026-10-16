#ifndef NOTELIST_H
#define NOTELIST_H

#include "note.h"
#include "notetype.h"
#include "ztabwidget.h"

#include <QDir>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>

class QFileSystemWatcher;

class NoteList : public QObject
{
    Q_OBJECT

public:
    explicit NoteList(QWidget *parent = nullptr);
    ~NoteList() override;

    [[nodiscard]] inline bool empty() const { return vec.isEmpty(); }
    [[nodiscard]] inline int count() const { return vec.size(); }
    [[nodiscard]] inline int last() const { return vec.size() - 1; }
    inline void setCurrent(int n) { tabs->setCurrentIndex(n); }
    [[nodiscard]] inline Note* current() const { return (current_index >= 0 && current_index < vec.size()) ? vec[current_index] : nullptr; }
    [[nodiscard]] inline Note* get(int i) const { return (i >= 0 && i < vec.size()) ? vec[i] : nullptr; }
    [[nodiscard]] inline int currentIndex() const { return current_index; }

    void create(Note::Type type, const QString &name = QString());

    // Search and Highlight APIs for the active note
    bool find(const QString &text,
              QTextDocument::FindFlags flags = {},
              bool wrap = false);
    bool findNextGlobal(const QString &text, QTextDocument::FindFlags flags);
    bool findPrevGlobal(const QString &text, QTextDocument::FindFlags flags);
    int findAll(const QString &text, QTextDocument::FindFlags flags = {});
    bool findInCurrent(const QString &text, 
                        QTextDocument::FindFlags flags = {}, bool wrap = false);
    void highlightInCurrent(const QString &text, 
                            QTextDocument::FindFlags flags = {});

    [[nodiscard]] inline ZTabWidget* getWidget() const { return tabs; }
    void retranslate(const QLocale &locale);
    [[nodiscard]] inline const QMap<Note::Type, NoteType>& noteTypes() const { return note_types; }
    void registerType(Note::Type id, const QString &title,
                      const QString &description, const QString &big_icon_path,
                      const QString &small_icon_path, const QString &extensions,
                      bool visible = true);

public slots:
    void fileModifiedExternally(const QString &path);
    void removeCurrentNote();
    void renameCurrentNote();
    void saveAll();

signals:
    void currentNoteChanged(int old_index, int new_index);

private slots:
    void currentTabChanged(int index);
    void showExtensionsChanged(bool show_extensions);
    void tabPositionChanged();
    void notesPathChanged();
    void scanForNewFiles();
    void changeNoteName(int tabIndex);

private:
    void initNoteTypes();
    Note::Type getType(const QFileInfo &fileinfo) const;
    Note* add(const QFileInfo &fileinfo, bool set_current = true);
    void remove(int i);
    void rename(int index, const QString &title);
    void move(const QString &newPath, const QString &oldPath);

    const QString FILE_WITH_CURSOR_POSITIONS;
    QWidget *parentWidget{nullptr};
    QMap<Note::Type, NoteType> note_types;
    QMap<QString, Note::Type> NOTE_TYPE_MAP;
    QSet<QString> notes_filenames;
    QList<Note*> vec;
    int current_index{-1};
    ZTabWidget *tabs{nullptr};
    QFileSystemWatcher *watcher{nullptr};
    QDir dir;
};

#endif // NOTELIST_H
