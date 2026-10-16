#ifndef NOTECREATEBUTTON_H
#define NOTECREATEBUTTON_H

#include <QPushButton>
#include "notetype.h"

class NoteCreateButton : public QPushButton
{
    Q_OBJECT

public:
    explicit NoteCreateButton(const NoteType &ntype, QWidget *parent = nullptr);
    ~NoteCreateButton() override = default;

    [[nodiscard]] const NoteType& noteType() const { return m_noteType; }

private:
    NoteType m_noteType;
};

#endif // NOTECREATEBUTTON_H
