#ifndef HTMLNOTE_H
#define HTMLNOTE_H

#include "note.h"

class TextEdit;

class HtmlNote : public Note
{
    Q_OBJECT

public:
    HtmlNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent = nullptr);
    ~HtmlNote() override;

    void load() override;                    // Reading note's content
    void save(bool forced = false) override; // Saving note's content
    void copy() const override;              // Copying note's content to clipboard
    
    bool find(const QString &text, 
              QTextDocument::FindFlags flags = {}, 
              bool wrap = false) override;
              
    void highlightAll(const QString &text, 
                      QTextDocument::FindFlags flags = {}) override;

    QWidget* widget() override;
    void setSelFormat(const QTextCharFormat &format) override;
    QTextCharFormat getSelFormat() const override;
    bool isDocumentSupported() const override;
    QTextDocument* document() const override;

private slots:
    void currentCharFormatChanged(const QTextCharFormat &format);
    void notePastePlaintextChanged();

private:
    TextEdit *text_edit{nullptr};

signals:
    void formatChanged(const QTextCharFormat &format);
};

#endif // HTMLNOTE_H
