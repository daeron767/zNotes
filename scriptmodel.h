#ifndef SCRIPTMODEL_H
#define SCRIPTMODEL_H

#include <QStandardItemModel>
#include <QString>

class ScriptModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ScriptModel(QObject *parent = nullptr);
    ~ScriptModel() override = default;

    void append(const QString &name = QString(),
                const QString &file = QString(),
                const QString &icon = QString());

    [[nodiscard]] QString getName(int i) const;
    [[nodiscard]] QString getFile(int i) const;
    [[nodiscard]] QString getIcon(int i) const;

    [[nodiscard]] QVariant headerData(int section,
                                      Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
};

#endif // SCRIPTMODEL_H
