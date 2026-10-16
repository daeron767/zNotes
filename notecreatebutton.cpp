#include "notecreatebutton.h"

NoteCreateButton::NoteCreateButton(const NoteType &ntype, QWidget *parent)
    : QPushButton(parent),
      m_noteType(ntype)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setIcon(m_noteType.icon());
    setIconSize(QSize(64, 64));

    const QString title = QString("%1\n\n%2")
                              .arg(m_noteType.title(), m_noteType.description());
    setText(title);
}
