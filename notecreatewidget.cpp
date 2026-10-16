#include "notecreatewidget.h"
#include "ui_notecreatewidget.h"
#include "notelist.h"
#include "notecreatebutton.h"

#include <QCloseEvent>
#include <QShowEvent>

NoteCreateWidget::NoteCreateWidget(NoteList *note_list, QWidget *parent)
    : QFrame(parent),
      ui(new Ui::NoteCreateWidget),
      m_notes(note_list)
{
    ui->setupUi(this);

    if (m_notes)
    {
        for (const NoteType &note_type : m_notes->noteTypes())
        {
            if (!note_type.visible())
                continue;

            auto *button = new NoteCreateButton(note_type, this);

            const int typeId = static_cast<int>(note_type.id());
            connect(button, &QPushButton::clicked, this, [this, typeId]() {
                onButtonClicked(typeId);
            });

            ui->noteTypesLayout->addWidget(button);
        }
    }
}

NoteCreateWidget::~NoteCreateWidget()
{
    delete ui;
}

void NoteCreateWidget::showEvent(QShowEvent *event)
{
    ui->noteNameEdit->clear();
    ui->noteNameEdit->setFocus();
    QFrame::showEvent(event);
}

void NoteCreateWidget::onButtonClicked(int id)
{
    if (m_notes)
    {
        m_notes->create(static_cast<Note::Type>(id), ui->noteNameEdit->text().trimmed());
    }
    close();
}

void NoteCreateWidget::closeEvent(QCloseEvent *event)
{
    emit closed(true);
    QFrame::closeEvent(event);
}

void NoteCreateWidget::changeEvent(QEvent *e)
{
    QFrame::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
}
