#ifndef SHARED_H
#define SHARED_H

#include <QList>
#include <QPointer>
#include <QShortcut>
#include <QString>

class Shared
{
public:
    Shared() = delete; // Utility class; prevent instantiation

    [[nodiscard]] static const QList<QShortcut*>& shortcuts();
    static void addShortcut(QShortcut *shortcut, const QString &comment = QString());

private:
    static QList<QShortcut*> p_shortcuts;
};

#endif // SHARED_H
