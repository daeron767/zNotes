#include "scriptmodel.h"

ScriptModel::ScriptModel(QObject *parent)
    : QStandardItemModel(0, 3, parent)
{
}

void ScriptModel::append(const QString &name, const QString &file, const QString &icon)
{
    const int r = rowCount();
    insertRow(r);
    setData(index(r, 0), name);
    setData(index(r, 1), file);
    setData(index(r, 2), icon);
}

QString ScriptModel::getName(int i) const
{
    if (i < 0 || i >= rowCount())
        return QString();
    return data(index(i, 0)).toString();
}

QString ScriptModel::getFile(int i) const
{
    if (i < 0 || i >= rowCount())
        return QString();
    return data(index(i, 1)).toString();
}

QString ScriptModel::getIcon(int i) const
{
    if (i < 0 || i >= rowCount())
        return QString();
    return data(index(i, 2)).toString();
}

QVariant ScriptModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0: return tr("Name");
            case 1: return tr("File");
            case 2: return tr("Icon");
            default: return QVariant();
        }
    }

    return QStandardItemModel::headerData(section, orientation, role);
}
