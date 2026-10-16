#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>
#include "highlighter.h"

class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = nullptr);
    ~TextEdit() override = default;

    // Search and Highlight APIs matching Note interface
    bool findText(const QString &text, 
                  QTextDocument::FindFlags flags = {}, 
                  bool wrap = false);

    void highlightAll(const QString &text, 
                      QTextDocument::FindFlags flags = {});

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private slots:
    void fontChanged();
    void linkOpenChanged();

private:
    void updateExtraSelections();

    Highlighter *highlighter{nullptr};
    QList<QTextEdit::ExtraSelection> searchSelections;
    QList<QTextEdit::ExtraSelection> linkSelections;
};

#endif // TEXTEDIT_H
