#include "shared.h"

// Define static class member
QList<QShortcut*> Shared::p_shortcuts;

const QList<QShortcut*>& Shared::shortcuts()
{
    return p_shortcuts;
}

void Shared::addShortcut(QShortcut *shortcut, const QString &comment)
{
    if (!shortcut)
        return;

    shortcut->setWhatsThis(comment);

    if (!p_shortcuts.contains(shortcut))
    {
        p_shortcuts.append(shortcut);

        // Automatically remove pointer from list if QShortcut is destroyed
        QObject::connect(shortcut, &QObject::destroyed, [] (QObject *obj) {
            p_shortcuts.removeAll(static_cast<QShortcut*>(obj));
        });
    }
}
