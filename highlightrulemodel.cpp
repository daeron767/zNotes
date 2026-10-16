#include "highlightrulemodel.h"
#include <utility>

HighlightRuleModel::HighlightRuleModel(QObject *parent, const QVector<HighlightRule> &rules)
    : QAbstractTableModel(parent), m_rules(rules)
{
}

int HighlightRuleModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 2;
}

int HighlightRuleModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rules.size();
}

QVariant HighlightRuleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0: return tr("Pattern");
            case 1: return tr("Color");
            default: break;
        }
    }
    else if (orientation == Qt::Vertical)
    {
        return section + 1;
    }
    return QVariant();
}

QVariant HighlightRuleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rules.size())
        return QVariant();

    const auto &rule = m_rules.at(index.row());

    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            switch (index.column())
            {
                case 0: return rule.regexp;
                case 1: return rule.color.name();
                default: return QVariant();
            }

        case Qt::BackgroundRole:
            if (index.column() == 1)
                return rule.color;
            return QVariant();

        case Qt::CheckStateRole:
            if (index.column() == 0)
                return rule.enabled ? Qt::Checked : Qt::Unchecked;
            return QVariant();

        default:
            return QVariant();
    }
}

bool HighlightRuleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rules.size())
        return false;

    auto &rule = m_rules[index.row()];

    if (role == Qt::CheckStateRole && index.column() == 0)
    {
        rule.enabled = (value.toInt() == Qt::Checked || value.toBool());
        emit dataChanged(index, index, {Qt::CheckStateRole});
        return true;
    }

    if (role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 0:
                rule.regexp = QRegularExpression(value.toString());
                emit dataChanged(index, index, {Qt::EditRole, Qt::DisplayRole});
                return true;
            case 1:
                rule.color = QColor(value.toString());
                emit dataChanged(index, index, {Qt::EditRole, Qt::DisplayRole, Qt::BackgroundRole});
                return true;
            default:
                break;
        }
    }

    return false;
}

Qt::ItemFlags HighlightRuleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;

    if (index.column() == 0)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

void HighlightRuleModel::appendRow()
{
    const int newRow = m_rules.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_rules.append(HighlightRule());
    endInsertRows();
}

bool HighlightRuleModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0 || (row + count) > m_rules.size())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    m_rules.remove(row, count);
    endRemoveRows();
    return true;
}

void HighlightRuleModel::removeRow(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    removeRows(index.row(), 1);
}

void HighlightRuleModel::up(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int row = index.row();
    if (row <= 0)
        return;

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
    std::swap(m_rules[row], m_rules[row - 1]);
    endMoveRows();
}

void HighlightRuleModel::down(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int row = index.row();
    if (row < 0 || row >= m_rules.size() - 1)
        return;

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + 2);
    std::swap(m_rules[row], m_rules[row + 1]);
    endMoveRows();
}
