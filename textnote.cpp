#include "textnote.h"
#include "textedit.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>

TextNote::TextNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent)
    : Note(fileinfo, type_new, parent)
{
    text_edit = new TextEdit();
    text_edit->setAcceptRichText(false);

    load(); // Load note content from disk

    // Modern C++ signal-slot syntax
    connect(text_edit, &QTextEdit::textChanged, this, &TextNote::contentChanged);
}

TextNote::~TextNote()
{
    delete text_edit;
}

void TextNote::load()
{
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return;
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#else
    in.setEncoding(QStringConverter::Utf8);
#endif

    // Block signals so populating the editor doesn't mark the note as modified
    const bool oldState = text_edit->signalsBlocked();
    text_edit->blockSignals(true);
    text_edit->setPlainText(in.readAll());
    text_edit->blockSignals(oldState);

    file.close();
    content_changed = false;
}

void TextNote::save(bool forced)
{
    if (!(content_changed || forced)) {
        return;
    }

    file.close();
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    out << text_edit->toPlainText();
    file.close();
    content_changed = false;
}

QWidget* TextNote::widget()
{
    return text_edit;
}

bool TextNote::isDocumentSupported() const
{
    return true;
}

QTextDocument* TextNote::document() const
{
    return text_edit->document();
}

void TextNote::copy() const
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text_edit->toPlainText());
    }
}

bool TextNote::find(const QString &text, QTextDocument::FindFlags flags, bool wrap)
{
    if (!text_edit || text.isEmpty())
        return false;
    bool found = text_edit->find(text, flags);
    if (!found && wrap) {
        QTextCursor cursor = text_edit->textCursor();
        cursor.movePosition((flags & QTextDocument::FindBackward) ? QTextCursor::End : QTextCursor::Start);
        text_edit->setTextCursor(cursor);
        found = text_edit->find(text, flags);
    }
    if (found) {
        text_edit->ensureCursorVisible();
    }
    return found;
}

void TextNote::highlightAll(const QString &text, QTextDocument::FindFlags flags)
{
    if (!text_edit)
        return;

    text_edit->highlightAll(text, flags);
}
