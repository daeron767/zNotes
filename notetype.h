#ifndef NOTETYPE_H
#define NOTETYPE_H

#include "note.h"

#include <QIcon>
#include <QString>
#include <QStringList>

class NoteType
{
public:
    NoteType() = default;
    NoteType(Note::Type id,
             const QString &title,
             const QString &description,
             const QString &big_icon_path,
             const QString &small_icon_path,
             const QString &extensions,
             bool visible = true);

    [[nodiscard]] inline Note::Type id() const { return note_id; }
    [[nodiscard]] inline const QString& title() const { return note_title; }
    [[nodiscard]] inline const QString& description() const { return note_description; }
    [[nodiscard]] inline const QIcon& icon() const { return big_icon; }
    [[nodiscard]] inline const QIcon& smallIcon() const { return small_icon; }
    [[nodiscard]] inline bool visible() const { return is_visible; }
    [[nodiscard]] inline const QStringList& extensions() const { return exts; }
    [[nodiscard]] inline QString defaultExtension() const { return exts.isEmpty() ? QString() : exts.first(); }

private:
    Note::Type note_id{Note::type_text};
    QStringList exts;
    QString note_title;
    QString note_description;
    QIcon big_icon;
    QIcon small_icon;
    bool is_visible{true};
};

#endif // NOTETYPE_H
