#include "todomodel.h"

#include <QCheckBox>
#include <QDataStream>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QStringList>

#define NOTE_TODO_TASK_MIME "application/znotes.content.list"

// Helper function to update/append XML text nodes safely
static void updateChildNode(QDomDocument &document, QDomNode &node, const QString &tag_name, const QString &value)
{
    QDomElement old_element = node.firstChildElement(tag_name);
    QDomElement new_element = document.createElement(tag_name);
    QDomText new_element_text = document.createTextNode(value);
    new_element.appendChild(new_element_text);

    if (!old_element.isNull())
        node.replaceChild(new_element, old_element);
    else
        node.appendChild(new_element);
}

// Helper function for human-readable deadline formatting
static QString getDateGap(const QDateTime &dest_date)
{
    if (dest_date.isNull())
        return QString();

    const QDateTime current = QDateTime::currentDateTime();
    const qint64 days_gap = current.daysTo(dest_date);
    const qint64 secs_gap = current.secsTo(dest_date);

    if (secs_gap < 0)
        return QObject::tr("expired");

    if (days_gap > 0)
        return QObject::tr("%n day(s)", "", static_cast<int>(days_gap));
    if (secs_gap >= 3600)
        return QObject::tr("%n hour(s)", "", static_cast<int>(secs_gap / 3600));
    if (secs_gap >= 60)
        return QObject::tr("%n minute(s)", "", static_cast<int>(secs_gap / 60));

    return QObject::tr("%n second(s)", "", static_cast<int>(secs_gap));
}

//------------------------------------------------------------------------------
// Task Implementation
//------------------------------------------------------------------------------

Task::Task(QDomDocument *document, const QDomNode &node, int row, Task *parent)
    : _document(document), _node(node), _row(row), _parent(parent)
{
    const QDomElement element = _node.toElement();
    if (!element.isNull())
    {
        if (element.hasAttribute("time_a"))
            _date_start = QDateTime::fromSecsSinceEpoch(element.attribute("time_a").toLongLong());

        if (element.hasAttribute("time_l"))
            _date_limit = QDateTime::fromSecsSinceEpoch(element.attribute("time_l").toLongLong());

        if (element.hasAttribute("time_d"))
        {
            _date_stop = QDateTime::fromSecsSinceEpoch(element.attribute("time_d").toLongLong());
            _done = true;
        }

        if (element.hasAttribute("priority"))
        {
            const QString priority_text = element.attribute("priority");
            if (priority_text == "low")
                _priority = Low;
            else if (priority_text == "high")
                _priority = High;
            else
                _priority = Medium;
        }

        const QDomNodeList children = _node.childNodes();
        for (int i = 0; i < children.count(); ++i)
        {
            const QDomNode child_node = children.item(i);
            const QDomElement child_element = child_node.toElement();

            if (child_element.tagName() == "title")
                _title = child_element.text();
            else if (child_element.tagName() == "comment")
                _comment = child_element.text();
            else if (child_element.tagName() == "task")
                _subtasks.append(new Task(_document, child_node, _subtasks.size(), this));
        }
    }
}

Task::~Task()
{
    qDeleteAll(_subtasks);
    _subtasks.clear();
}

void Task::updateSubtaskRows()
{
    for (int i = 0; i < _subtasks.size(); ++i)
    {
        _subtasks[i]->setRow(i);
    }
}

void Task::insertSubTask(int pos)
{
    if (pos < 0 || pos > _subtasks.size())
        pos = _subtasks.size();

    QDomElement new_task_elem = _document->createElement("task");

    if (pos == _subtasks.size())
    {
        _node.appendChild(new_task_elem);
    }
    else
    {
        // Insert relative to the target subtask's DOM node rather than childNodes().at(pos)
        _node.insertBefore(new_task_elem, _subtasks.at(pos)->node());
    }

    Task *task = new Task(_document, new_task_elem, pos, this);
    task->setDateStart(QDateTime::currentDateTime());
    task->setTitle(QObject::tr("New task"));

    _subtasks.insert(pos, task);
    updateSubtaskRows();
}

void Task::removeSubTask(int pos)
{
    if (pos < 0 || pos >= _subtasks.size())
        return;

    Task *subtask = _subtasks.takeAt(pos);
    if (subtask)
    {
        _node.removeChild(subtask->node());
        delete subtask;
    }

    updateSubtaskRows();
}

void Task::setTitle(const QString &v)
{
    _title = v;
    updateChildNode(*_document, _node, "title", _title);
}

void Task::setComment(const QString &v)
{
    _comment = v;
    updateChildNode(*_document, _node, "comment", _comment);
}

void Task::setDateStart(const QDateTime &v)
{
    _date_start = v;
    _node.toElement().setAttribute("time_a", QString::number(_date_start.toSecsSinceEpoch()));
}

void Task::setDateStop(const QDateTime &v)
{
    _date_stop = v;
    _node.toElement().setAttribute("time_d", QString::number(_date_stop.toSecsSinceEpoch()));
}

void Task::setDateLimit(const QDateTime &v)
{
    _date_limit = v;
    _node.toElement().setAttribute("time_l", QString::number(_date_limit.toSecsSinceEpoch()));
}

void Task::setDone(bool v)
{
    _done = v;
    if (_done)
    {
        setDateStop(QDateTime::currentDateTime());
    }
    else
    {
        _node.toElement().removeAttribute("time_d");
        _date_stop = QDateTime();
    }
}

void Task::setPriority(Priority v)
{
    _priority = v;
    switch (_priority)
    {
    case Low:
        _node.toElement().setAttribute("priority", "low");
        break;
    case Medium:
        _node.toElement().setAttribute("priority", "medium");
        break;
    case High:
        _node.toElement().setAttribute("priority", "high");
        break;
    }
}

//------------------------------------------------------------------------------
// TodoModel Implementation
//------------------------------------------------------------------------------

TodoModel::TodoModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    _document = new QDomDocument();
    
    // Create root XML structure if empty
    QDomElement root = _document->createElement("tasks");
    _document->appendChild(root);

    // Ensure _root_task wraps a valid document pointer and element
    _root_task = new Task(_document, root, 0, nullptr);
}

TodoModel::~TodoModel()
{
    delete _root_task;
    delete _document;
}

QDomDocument* TodoModel::load(QFile &file)
{
    beginResetModel();

    // Safely cleanup old task tree BEFORE deleting document
    delete _root_task;
    _root_task = nullptr;

    delete _document;
    _document = new QDomDocument("ztodo");

    if (!_document->setContent(&file) || !_document->childNodes().count())
    {
        _document->appendChild(_document->createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'"));

        QDomElement elem = _document->createElement("ztodo");
        elem.setAttribute("version", "1.0");
        _document->appendChild(elem);
    }

    QDomElement root_element = _document->documentElement();
    _root_task = new Task(_document, root_element, 0, nullptr);

    endResetModel();
    return _document;
}

bool TodoModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0)
        return false;

    Task *parentTask = parent.isValid() ? static_cast<Task*>(parent.internalPointer()) : _root_task;
    if (!parentTask)
        return false;

    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i) {
        // insertSubTask handles DOM creation, task allocation, positioning, and row indexing
        parentTask->insertSubTask(row + i);
    }

    endInsertRows();
    return true;
}

bool TodoModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Task *task_parent = getTask(parent);
    if (!task_parent)
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        task_parent->removeSubTask(row);
    }
    endRemoveRows();
    return true;
}

Qt::DropActions TodoModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList TodoModel::mimeTypes() const
{
    return {NOTE_TODO_TASK_MIME};
}

QMimeData* TodoModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes)
    {
        if (!index.isValid() || index.column() != 0)
            continue;

        Task *task = getTask(index);
        if (!task)
            continue;

        stream << task->title();
        stream << task->comment();
        stream << static_cast<qint64>(task->dateStart().toSecsSinceEpoch());
        stream << static_cast<qint64>(task->dateStop().toSecsSinceEpoch());
        stream << static_cast<qint64>(task->dateLimit().toSecsSinceEpoch());
        stream << task->done();
        stream << static_cast<int>(task->priority());
    }

    mimeData->setData(NOTE_TODO_TASK_MIME, encodedData);
    return mimeData;
}

bool TodoModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column)

    if (action == Qt::IgnoreAction)
        return true;
    if (!parent.isValid() || !data->hasFormat(NOTE_TODO_TASK_MIME) || action != Qt::MoveAction)
        return false;

    Task *task_parent = getTask(parent);
    if (!task_parent)
        return false;

    const QList<Task*> &task_list = task_parent->subtasks();
    if (row == -1)
        row = task_list.size();

    if (insertRow(row, parent))
    {
        Task *task = task_parent->subtasks().at(row);

        QByteArray encodedData = data->data(NOTE_TODO_TASK_MIME);
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        QString title, comment;
        qint64 date_start_ts{0}, date_stop_ts{0}, date_limit_ts{0};
        bool done{false};
        int priority{0};

        stream >> title >> comment
               >> date_start_ts >> date_stop_ts >> date_limit_ts
               >> done >> priority;

        task->setTitle(title);
        task->setComment(comment);

        if (date_start_ts > 0)
            task->setDateStart(QDateTime::fromSecsSinceEpoch(date_start_ts));
        if (done && date_stop_ts > 0)
            task->setDateStop(QDateTime::fromSecsSinceEpoch(date_stop_ts));
        if (date_limit_ts > 0)
            task->setDateLimit(QDateTime::fromSecsSinceEpoch(date_limit_ts));

        task->setDone(done);
        task->setPriority(static_cast<Task::Priority>(priority));

        return true;
    }

    return false;
}

int TodoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    Task *task = getTask(parent);
    return task ? task->subtasks().size() : 0;
}

int TodoModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 7;
}

Task* TodoModel::getTask(const QModelIndex &index) const
{
    if (index.isValid())
    {
        Task *task = static_cast<Task*>(index.internalPointer());
        if (task)
            return task;
    }
    return _root_task;
}

QVariant TodoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Task *task = static_cast<Task*>(index.internalPointer());
    if (!task)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:
        {
            const int child_count = task->subtasks().count();
            if (child_count == 0)
                return task->title();

            int child_done = 0;
            for (const Task *subtask : task->subtasks())
            {
                if (subtask->done())
                    ++child_done;
            }
            return QString("%1 (%2/%3)").arg(task->title()).arg(child_done).arg(child_count);
        }
        case 1:
            return (!task->done()) ? getDateGap(task->dateLimit()) : QString();
        case 2:
            return task->dateStart();
        case 3:
            return task->dateStop();
        case 4:
            return task->dateLimit();
        case 5:
            return static_cast<int>(task->priority());
        case 6:
            return task->comment();
        default:
            return QVariant();
        }
    }
    else if (role == Qt::EditRole)
    {
        switch (index.column())
        {
        case 0:
            return task->title();
        case 1:
            return (!task->done()) ? task->dateLimit() : QVariant();
        case 2:
            return task->dateStart();
        case 3:
            return task->dateStop();
        case 4:
            return task->dateLimit();
        case 6:
            return task->comment();
        default:
            return QVariant();
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        if (index.column() == 0)
            return task->done() ? Qt::Checked : Qt::Unchecked;
    }
    else if (role == Qt::FontRole)
    {
        if (task->done())
        {
            QFont font;
            font.setStrikeOut(true);
            return font;
        }
        return QFont();
    }
    else if (role == Qt::ForegroundRole)
    {
        QPalette::ColorGroup colorgroup = task->done() ? QPalette::Disabled : QPalette::Normal;
        return QPalette().color(colorgroup, QPalette::Text);
    }

    return QVariant();
}

bool TodoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    Task *task = static_cast<Task*>(index.internalPointer());
    if (!task)
        return false;

    switch (index.column())
    {
    case 0:
        if (role == Qt::CheckStateRole)
        {
            const bool isChecked = (value.toInt() == Qt::Checked) || value.toBool();
            task->setDone(isChecked);
            emit dataChanged(index, index);
            return true;
        }
        else if (role == Qt::EditRole)
        {
            task->setTitle(value.toString());
            emit dataChanged(index, index);
            return true;
        }
        break;

    case 4:
        if (role == Qt::EditRole)
        {
            task->setDateLimit(value.toDateTime());
            emit dataChanged(index, index);

            const QModelIndex date_display_index = index.sibling(index.row(), 1);
            emit dataChanged(date_display_index, date_display_index);
            return true;
        }
        break;

    case 6:
        if (role == Qt::EditRole)
        {
            task->setComment(value.toString());
            emit dataChanged(index, index);
            return true;
        }
        break;

    default:
        break;
    }

    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags TodoModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    const Qt::ItemFlags baseFlags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    switch (index.column())
    {
    case 0:
        return baseFlags | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    case 4:
    case 6:
        return baseFlags | Qt::ItemIsEditable;
    default:
        return baseFlags;
    }
}

QVariant TodoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return tr("Task");
        case 1:
            return tr("Time Left");
        case 2:
            return tr("Created");
        case 3:
            return tr("Completed");
        case 4:
            return tr("Due Date");
        case 5:
            return tr("Priority");
        case 6:
            return tr("Comment");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

QModelIndex TodoModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Task *parent_item = getTask(parent);
    if (!parent_item)
        return QModelIndex();

    if (row >= 0 && row < parent_item->subtasks().size())
    {
        Task *child_item = parent_item->subtasks().at(row);
        if (child_item)
            return createIndex(row, column, child_item);
    }

    return QModelIndex();
}

QModelIndex TodoModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    Task *child_item = static_cast<Task*>(child.internalPointer());
    if (!child_item)
        return QModelIndex();

    Task *parent_item = child_item->parent();
    if (!parent_item || parent_item == _root_task)
        return QModelIndex();

    return createIndex(parent_item->row(), 0, parent_item);
}

//------------------------------------------------------------------------------
// TodoProxyModel Implementation
//------------------------------------------------------------------------------

TodoProxyModel::TodoProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void TodoProxyModel::hideDoneTasks(bool hide)
{
    if (hide_done_tasks != hide)
    {
        beginFilterChange();
        hide_done_tasks = hide;
        endFilterChange();
    }
}

bool TodoProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex source_index = sourceModel()->index(source_row, 0, source_parent);
    if (!source_index.isValid())
        return false;

    if (hide_done_tasks)
    {
        const int checkState = source_index.data(Qt::CheckStateRole).toInt();
        if (checkState == Qt::Checked)
            return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
