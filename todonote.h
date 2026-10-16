#ifndef TODONOTE_H
#define TODONOTE_H

#include "note.h"

class QTreeView;
class TodoModel;
class TodoProxyModel;
class QScrollArea;
class TextEdit;
class QLayout;
class QDataWidgetMapper;
class QDateTimeEdit;
class QLabel;
class QCheckBox;
class QMenu;
#include <QScrollArea>
#include <QHideEvent>
#include <QApplication>

class TodoScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit TodoScrollArea(QWidget *parent = nullptr) : QScrollArea(parent) {}

protected:
    void hideEvent(QHideEvent *event) override
    {
        if (QWidget *fw = QApplication::focusWidget()) {
            if (this->isAncestorOf(fw)) {
                fw->clearFocus();
            }
        }
        QScrollArea::hideEvent(event);
    }
};

class TodoNote : public Note
{
    Q_OBJECT
public:
    explicit TodoNote(const QFileInfo& fileinfo, Note::Type type_new, QObject* parent = nullptr);
    ~TodoNote() override;

    void load() override;
    void save(bool forced = false) override;
    void copy() const override;
    QWidget* widget() override;
    void retranslate(const QLocale& locale) override;
    bool isDocumentSupported() const override;
    QTextDocument* document() const override;

private slots:
    void currentTaskChanged(const QModelIndex& proxy_index);
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void contextMenuRequested(const QPoint& pos);
    void insertTask();
    void insertSubTask();
    void removeTask();
    void hideCompletedTasks();
    void noteDateLimitChanged(const QDateTime& dt);
    void noteLimitChanged(bool checked);

private:
    QTreeView* tree_view{nullptr};
    TodoModel* model{nullptr};
    TodoProxyModel* proxy_model{nullptr};
    TodoScrollArea* area{nullptr};
    TextEdit* text_edit{nullptr};
    QWidget* extra_widget{nullptr};
    QLayout* main_layout{nullptr};
    QLayout* grid_layout{nullptr};
    QLayout* extra_layout{nullptr};
    QDataWidgetMapper* mapper{nullptr};
    QLabel* lb_date_start{nullptr};
    QLabel* lb_date_stop{nullptr};
    QDateTimeEdit* dt_date_limit{nullptr};
    QLabel* lb_date_0{nullptr};
    QLabel* lb_date_1{nullptr};
    QCheckBox* cb_date_limit{nullptr};
    QMenu* menu_context{nullptr};

};

#endif // TODONOTE_H
