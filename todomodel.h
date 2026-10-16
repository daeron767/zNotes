#ifndef TODOMODEL_H
#define TODOMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QList>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QString>
#include <QVariant>

class Task
{
public:
    enum Priority
    {
        Low,
        Medium,
        High
    };
    explicit Task(QDomDocument *document,
                  const QDomNode &node,
                  int row,
                  Task *parent = nullptr);
    ~Task();
    // Disable copying to prevent invalid tree nodes
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    [[nodiscard]] const QList<Task*>& subtasks() const { return _subtasks; }
    [[nodiscard]] Task* parent() const { return _parent; }
    [[nodiscard]] int row() const { return _row; }
    void setRow(int row) { _row = row; }
    [[nodiscard]] const QDomNode& node() const { return _node; }
    [[nodiscard]] const QString& title() const { return _title; }
    [[nodiscard]] const QString& comment() const { return _comment; }
    [[nodiscard]] const QDateTime& dateStart() const { return _date_start; }
    [[nodiscard]] const QDateTime& dateStop() const { return _date_stop; }
    [[nodiscard]] const QDateTime& dateLimit() const { return _date_limit; }
    [[nodiscard]] bool done() const { return _done; }
    [[nodiscard]] bool limited() const { return !_date_limit.isNull(); }
    [[nodiscard]] Priority priority() const { return _priority; }

    void setTitle(const QString &v);
    void setComment(const QString &v);
    void setDateStart(const QDateTime &v);
    void setDateStop(const QDateTime &v);
    void setDateLimit(const QDateTime &v);
    void setDone(bool v);
    void setPriority(Priority v);
    void insertSubTask(int pos = -1);
    void removeSubTask(int pos);

private:
    void updateSubtaskRows();
    QDomDocument *_document{nullptr};
    QDomNode _node;
    int _row{0};
    Task *_parent{nullptr};
    QList<Task*> _subtasks;
    QString _title;
    QString _comment;
    QDateTime _date_start;
    QDateTime _date_stop;
    QDateTime _date_limit;
    Priority _priority{Medium};
    bool _done{false};
};

class TodoModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TodoModel(QObject *parent = nullptr);
    ~TodoModel() override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QDomDocument* document() const { return _document; }
    QDomDocument* load(QFile &file);
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList &indexes) const override;

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] Task* getTask(const QModelIndex &index) const;

private:
    QDomDocument *_document{nullptr};
    Task *_root_task{nullptr};
};

class TodoProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TodoProxyModel(QObject *parent = nullptr);
    ~TodoProxyModel() override = default;
    void hideDoneTasks(bool hide = true);
    [[nodiscard]] bool isHidingDoneTasks() const { return hide_done_tasks; }

protected:
    [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    bool hide_done_tasks{false};
};

#endif // TODOMODEL_H 
