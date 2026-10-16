#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QAbstractButton>
#include <QModelIndex>
#include <memory>

class HighlightRuleModel;
class ItemModel;
class ItemToolbarModel;
class ToolbarItems;

namespace Ui {
    class configDialog;
}

class configDialog : public QDialog
{
    Q_OBJECT

public:
    explicit configDialog(QWidget *parent = nullptr);
    ~configDialog() override;

    void changeTabToFirst();
    void changeTabToCommands();

    void saveSettings();

private slots:
    void on_buttonBox_clicked(QAbstractButton* button);
    void on_btn_NotesPath_clicked();
    void on_btn_FontChange_clicked();

    void on_btn_ScriptAdd_clicked();
    void on_btn_ScriptRemove_clicked();
    void on_butActionAdd_clicked();
    void on_butActionRemove_clicked();
    void on_butActionTop_clicked();
    void on_butActionBottom_clicked();

    void currentToolbarActionChanged(QModelIndex index, QModelIndex previous);
    void currentListActionChanged(QModelIndex index, QModelIndex previous);
    void currentHighlightRuleModelRowChanged(QModelIndex index, QModelIndex previous = QModelIndex());

    void on_highlightRuleAddButton_clicked();
    void on_highlightRuleRemoveButton_clicked();
    void on_highlightRuleUpButton_clicked();
    void on_highlightRuleDownButton_clicked();

    void changeEvent(QEvent *e) override;

private:
    Ui::configDialog *m_ui;

    HighlightRuleModel *highlight_rule_model = nullptr;
    ItemModel *m_items = nullptr;
    ItemToolbarModel *mt_items = nullptr;
    std::unique_ptr<ToolbarItems> t_items;

    // Font preview widgets
    QLabel* lb_FontHtmlExample = nullptr;
    QPushButton* btn_FontHtmlChange = nullptr;
};

#endif // CONFIGDIALOG_H
