#ifndef NOTECREATEWIDGET_H
#define NOTECREATEWIDGET_H

#include <QFrame>

class NoteList;

namespace Ui {
class NoteCreateWidget;
}

class NoteCreateWidget : public QFrame
{
    Q_OBJECT

public:
    explicit NoteCreateWidget(NoteList *note_list, QWidget *parent = nullptr);
    ~NoteCreateWidget() override;

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *e) override;

signals:
    void closed(bool state);

private slots:
    void onButtonClicked(int id);

private:
    Ui::NoteCreateWidget *ui;
    NoteList *m_notes{nullptr};
};

#endif // NOTECREATEWIDGET_H
