#include "htmlnote.h"
#include "textedit.h"
#include "settings.h"

#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QTextStream>

HtmlNote::HtmlNote(const QFileInfo &fileinfo, Note::Type type_new, QObject *parent)
    : Note(fileinfo, type_new, parent)
{
    text_edit = new TextEdit();

    load(); // Load note content from disk

    // Signal-slot connections
    connect(text_edit, &QTextEdit::textChanged, this, &HtmlNote::contentChanged);
    connect(text_edit, &QTextEdit::currentCharFormatChanged, this, &HtmlNote::currentCharFormatChanged);

    text_edit->setAcceptRichText(!settings.getNotePastePlaintext());
    connect(&settings, &Settings::NotePastePlaintextChanged, this, &HtmlNote::notePastePlaintextChanged);
}

HtmlNote::~HtmlNote()
{
    delete text_edit;
}

void HtmlNote::load()
{
    file.close();
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#else
        in.setEncoding(QStringConverter::Utf8);
#endif
        QString html_content = in.readAll();
        // Strip explicit point, pixel, em, and rem font-size locks from <body> and inline CSS
        // Preserves relative HTML font size keywords (x-small, small, medium, large, x-large, xx-large)
        static const QRegularExpression sizeLockRegex(
            QStringLiteral("font-size\\s*:\\s*\\d+(\\.\\d+)?(pt|px|em|rem)\\s*;?"),
            QRegularExpression::CaseInsensitiveOption
        );
        html_content.remove(sizeLockRegex);
        // Temporarily block signals so setting HTML doesn't trigger contentChanged()
        const bool old_signals = text_edit->signalsBlocked();
        text_edit->blockSignals(true);
        text_edit->setHtml(html_content);
        text_edit->blockSignals(old_signals);
        file.close();
        content_changed = false;
    }
    else if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.close(); // If file doesn't exist, create it
    }
}

void HtmlNote::save(bool forced)
{
    if (!(content_changed || forced))
        return;

    file.close();
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    out << text_edit->toHtml();
    file.close();
    content_changed = false;
}

QWidget* HtmlNote::widget()
{
    return text_edit;
}

void HtmlNote::copy() const
{
    if (QClipboard *clipboard = QApplication::clipboard())
    {
        clipboard->setText(text_edit->toHtml());
    }
}

bool HtmlNote::find(const QString &text, QTextDocument::FindFlags flags, bool wrap)
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

void HtmlNote::highlightAll(const QString &text, QTextDocument::FindFlags flags)
{
    if (!text_edit)
        return;

    text_edit->highlightAll(text, flags);
}

void HtmlNote::currentCharFormatChanged(const QTextCharFormat &format)
{
    emit formatChanged(format);
}

void HtmlNote::setSelFormat(const QTextCharFormat &format)
{
    if (!text_edit)
        return;

    QTextCursor cursor = text_edit->textCursor();
    if (!cursor.hasSelection())
    {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
}

QTextCharFormat HtmlNote::getSelFormat() const
{
    if (!text_edit)
        return {};

    return text_edit->textCursor().charFormat();
}

void HtmlNote::notePastePlaintextChanged()
{
    if (text_edit)
    {
        text_edit->setAcceptRichText(!settings.getNotePastePlaintext());
    }
}

bool HtmlNote::isDocumentSupported() const
{
    return true;
}

QTextDocument* HtmlNote::document() const
{
    return text_edit ? text_edit->document() : nullptr;
}

