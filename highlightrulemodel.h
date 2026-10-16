#ifndef HIGHLIGHTRULEMODEL_H
#define HIGHLIGHTRULEMODEL_H

#include <QAbstractTableModel>
#include <QVector>

#include "settings.h"

class HighlightRuleModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HighlightRuleModel(QObject *parent, const QVector<HighlightRule> &rules);
    ~HighlightRuleModel() override = default;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void appendRow();
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    void removeRow(const QModelIndex &index);

    void up(const QModelIndex &index);
    void down(const QModelIndex &index);

    const QVector<HighlightRule>& getRules() const { return m_rules; }

private:
    QVector<HighlightRule> m_rules;
};

#endif // HIGHLIGHTRULEMODEL_H
