#include "todonote.h"
#include "textedit.h"
#include "todomodel.h"
#include "settings.h"

#include <QClipboard>
#include <QApplication>

#include <QTreeView>
#include <QHeaderView>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDataWidgetMapper>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QLabel>
#include <QCheckBox>
#include <QMenu>
#include <QTextStream>

enum
{
    TODO_ACTION_INSERT,
    TODO_ACTION_INSERT_SUB,
    TODO_ACTION_REMOVE,
    //TODO_ACTION_HIDE_COMPLETED,
    TODO_ACTION_COUNT,
};

TodoNote::TodoNote(const QFileInfo& fileinfo, Note::Type type_new, QObject* parent)
    : Note(fileinfo, type_new, parent)
{
    text_edit = new TextEdit();
    text_edit->setAcceptRichText(false);
    text_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    model = new TodoModel(this);
    proxy_model = new TodoProxyModel(this);
    proxy_model->setSourceModel(model);

    tree_view = new QTreeView();
    tree_view->setModel(proxy_model);
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    tree_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    tree_view->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    tree_view->header()->hide();
    tree_view->setDragEnabled(true);
    tree_view->setAcceptDrops(true);
    tree_view->setDropIndicatorShown(true);
    tree_view->setDragDropMode(QAbstractItemView::InternalMove);
    tree_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    connect(proxy_model, &QAbstractItemModel::dataChanged,
        this, &TodoNote::onDataChanged);
    connect(tree_view->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &TodoNote::currentTaskChanged);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setAnimated(true);

    connect(tree_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequested(QPoint)));

    menu_context = new QMenu(tree_view);
    menu_context->addAction(tr("Insert new task"), this, SLOT(insertTask()));
    menu_context->addAction(tr("Insert new subtask"), this, SLOT(insertSubTask()));
    menu_context->addAction(tr("Remove this task"), this, SLOT(removeTask()));

    for(int i=2; i<model->columnCount(); ++i)
        tree_view->setColumnHidden(i, true);

    lb_date_start = new QLabel;
    lb_date_0 = new QLabel(tr("Created: "), lb_date_start);
    lb_date_stop = new QLabel;
    lb_date_1 = new QLabel(tr("Completed: "), lb_date_stop);
    dt_date_limit = new QDateTimeEdit;
    dt_date_limit->setCalendarPopup(true);
    cb_date_limit = new QCheckBox(tr("Limited: "), dt_date_limit);
    cb_date_limit->setCheckable(true);

    grid_layout = new QGridLayout();
    QGridLayout* l = qobject_cast<QGridLayout*>(grid_layout);
    l->addWidget(lb_date_0, 0, 0);
    l->addWidget(lb_date_start, 0, 1);
    l->addWidget(lb_date_1, 1, 0);
    l->addWidget(lb_date_stop, 1, 1);
    l->addWidget(cb_date_limit, 2, 0);
    l->addWidget(dt_date_limit, 2, 1);

    extra_layout = new QVBoxLayout;
    extra_layout->addItem(grid_layout);
    extra_layout->addWidget(text_edit);

    extra_widget = new QWidget;
    extra_widget->setLayout(extra_layout);
    extra_widget->hide();

    main_layout = new QVBoxLayout();
    main_layout->addWidget(tree_view);
    main_layout->addWidget(extra_widget);

    area = new TodoScrollArea();
    area->setLayout(main_layout);

    load(); // loading note's content

    mapper = new QDataWidgetMapper(this);
    mapper->setModel(proxy_model);
    mapper->addMapping(text_edit, 6, "plainText");
    mapper->addMapping(lb_date_start, 2, "text");
    mapper->addMapping(lb_date_stop, 3, "text");
    lb_date_start->setLocale(settings.getLocale());
    dt_date_limit->setLocale(settings.getLocale());
    dt_date_limit->calendarWidget()->setLocale(settings.getLocale());

    tree_view->setCurrentIndex(QModelIndex());
}

TodoNote::~TodoNote()
{
    delete area; // Parent widget cleans up child widgets & layouts automatically
}

void TodoNote::retranslate(const QLocale& locale)
{
    menu_context->actions()[TODO_ACTION_INSERT]->setText(tr("Insert new task"));
    menu_context->actions()[TODO_ACTION_INSERT_SUB]->setText(tr("Insert new subtask"));
    menu_context->actions()[TODO_ACTION_REMOVE]->setText(tr("Remove this task"));
    lb_date_0->setText(tr("Created: "));
    lb_date_1->setText(tr("Completed: "));
    cb_date_limit->setText(tr("Limited: "));
    lb_date_start->setLocale(locale);
    dt_date_limit->setLocale(locale);
    dt_date_limit->calendarWidget()->setLocale(locale);
}

void TodoNote::load()
{
    file.setFileName(absolutePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        model->load(file);
        file.close();
    }
}

void TodoNote::save(bool force)
{
    Q_UNUSED(force);
    if (!model || !model->document() || model->document()->isNull()) {
        return;
    }
    file.setFileName(absolutePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        model->document()->save(stream, 4);
        file.close();
    }
}

QWidget* TodoNote::widget()
{
    return area;
}

void TodoNote::copy() const
{
}

void TodoNote::contextMenuRequested(const QPoint& pos)
{
    QModelIndex index = tree_view->indexAt(pos);
    menu_context->actions()[TODO_ACTION_INSERT]->setVisible(!index.isValid());
    menu_context->actions()[TODO_ACTION_INSERT_SUB]->setVisible(index.isValid());
    menu_context->actions()[TODO_ACTION_REMOVE]->setEnabled(index.isValid());
    QPoint pos_global = tree_view->mapToGlobal(pos);
    menu_context->exec(pos_global);
}

void TodoNote::insertTask()
{
    QModelIndex index;
    int row = model->rowCount(index);
    model->insertRow(row, index);
    QModelIndex child_index = model->index(row, 0);
    QModelIndex proxy_index = proxy_model->mapFromSource(child_index);
    tree_view->setCurrentIndex(proxy_index);
    contentChanged();
    save();
}

void TodoNote::insertSubTask()
{
    QModelIndex proxy_index = tree_view->currentIndex();
    QModelIndex index = proxy_model->mapToSource(proxy_index);
    int row = model->rowCount(index);
    model->insertRow(row, index);
    QModelIndex child_index = index.model()->index(row, 0, index);
    proxy_index = proxy_model->mapFromSource(child_index);
    tree_view->setCurrentIndex(proxy_index);
    contentChanged();
    save();
}

void TodoNote::removeTask()
{
    QModelIndex proxy_index = tree_view->currentIndex();
    QModelIndex index = proxy_model->mapToSource(proxy_index);
    if(index.isValid()) {
        model->removeRow(index.row(), index.parent());
        contentChanged();
        save();
    }
}

void TodoNote::hideCompletedTasks()
{
}

// Called on selection change - updates editor panel, NO saving
void TodoNote::currentTaskChanged(const QModelIndex& proxy_index)
{
    extra_widget->setVisible(proxy_index.isValid());
    if (!proxy_index.isValid())
        return;

    mapper->setRootIndex(proxy_index.parent());
    mapper->setCurrentModelIndex(proxy_index);

    QModelIndex index = proxy_model->mapToSource(proxy_index);
    Task* task = static_cast<Task*>(index.internalPointer());

    // Temporarily disconnect date controls so setting values doesn't fire edit signals
    disconnect(dt_date_limit, &QDateTimeEdit::dateTimeChanged, this, &TodoNote::noteDateLimitChanged);
    disconnect(cb_date_limit, &QCheckBox::toggled, this, &TodoNote::noteLimitChanged);

    dt_date_limit->setDateTime(task->dateLimit());
    cb_date_limit->setChecked(task->limited());

    connect(dt_date_limit, &QDateTimeEdit::dateTimeChanged, this, &TodoNote::noteDateLimitChanged);
    connect(cb_date_limit, &QCheckBox::toggled, this, &TodoNote::noteLimitChanged);

    bool task_done = task->done();
    lb_date_1->setVisible(task_done);
    lb_date_stop->setVisible(task_done);
    cb_date_limit->setHidden(task_done);
    dt_date_limit->setHidden(task_done);
    dt_date_limit->setEnabled(task->limited());
}

// Called only when data in the model actually changes
void TodoNote::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    contentChanged();
    save();
}

void TodoNote::noteDateLimitChanged(const QDateTime& date)
{
    QModelIndex proxy_index = tree_view->currentIndex();
    QModelIndex index = proxy_model->mapToSource(proxy_index);
    if(!index.isValid()) return;
    QModelIndex date_limit_index = index.sibling(index.row(), 4);
    model->setData(date_limit_index, date, Qt::EditRole);
    contentChanged();
    save();
}

void TodoNote::noteLimitChanged(bool limited)
{
    QDateTime date = limited ? QDateTime::currentDateTime().addDays(7) : QDateTime();
    noteDateLimitChanged(date);
    contentChanged();
    save();
}

bool TodoNote::isDocumentSupported() const
{
    return false;
}

QTextDocument *TodoNote::document() const
{
    return nullptr;
}

