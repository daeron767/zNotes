#ifndef TEXTNOTE_H
#define TEXTNOTE_H

#include "note.h"

class TextEdit;

class TextNote : public Note
{
    Q_OBJECT

public:
    TextNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent = nullptr);
    ~TextNote() override;

    void load() override;                    // Reading note's content
    void save(bool forced = false) override; // Saving note's content
    void copy() const override;              // Copying note's content to clipboard
    
    bool find(const QString &text, 
              QTextDocument::FindFlags flags = {}, 
              bool wrap = false) override;
              
    void highlightAll(const QString &text, 
                      QTextDocument::FindFlags flags = {}) override;

    QWidget* widget() override;
    bool isDocumentSupported() const override;
    QTextDocument* document() const override;

private:
    TextEdit *text_edit{nullptr};
};

#endif // TEXTNOTE_H
