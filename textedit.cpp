#include "textedit.h"
#include "settings.h"

#include <QMouseEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QTextBlock>

static bool isOnLink(const QTextDocument &document, const QTextCursor &cursor, int &pos_start, int &pos_end, QUrl &linkUrl)
{
    Q_UNUSED(document);
    const QTextBlock block = cursor.block();
    if (!block.isValid()) return false;

    const QString text = block.text();
    const int posInBlock = cursor.position() - block.position();

    static const QRegularExpression urlRegex(
        R"((?:https?|ftp)://[^\s<>"{}|\\^`]+)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatchIterator it = urlRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int start = match.capturedStart();
        int end = match.capturedEnd();

        while (end > start && QString(".,;:!?)]}").contains(text.at(end - 1))) {
            --end;
        }

        if (posInBlock >= start && posInBlock <= end) {
            pos_start = block.position() + start;
            pos_end = block.position() + end;
            linkUrl = QUrl(text.mid(start, end - start));
            return linkUrl.isValid();
        }
    }
    return false;
}

TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    highlighter = new Highlighter(document());

    connect(&settings, &Settings::NoteHighlightChanged, highlighter, &Highlighter::rehighlight);
    connect(&settings, &Settings::NoteFontChanged, this, &TextEdit::fontChanged);
    connect(&settings, &Settings::NoteLinkOpenChanged, this, &TextEdit::linkOpenChanged);

    setFont(settings.getNoteFont());
    setMouseTracking(settings.getNoteLinksOpen());
    setWordWrapMode(QTextOption::WordWrap);
}

void TextEdit::updateExtraSelections()
{
    // Merge search highlights and link hover selections cleanly
    QList<QTextEdit::ExtraSelection> allSelections = searchSelections;
    allSelections.append(linkSelections);
    setExtraSelections(allSelections);
}

void TextEdit::mousePressEvent(QMouseEvent *e)
{
    const QPoint mousePos = e->position().toPoint();

    if (settings.getNoteLinksOpen() && e->buttons() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier))
    {
        int position_start = 0, position_end = 0;
        QUrl linkUrl;
        QTextCursor cursor = cursorForPosition(mousePos);

        if (isOnLink(*document(), cursor, position_start, position_end, linkUrl)) {
            QDesktopServices::openUrl(linkUrl);
            return;
        }
    }

    QTextEdit::mousePressEvent(e);
}

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint mousePos = e->position().toPoint();

    bool onLink = false;
    int position_start = 0, position_end = 0;
    QUrl linkUrl;

    if (settings.getNoteLinksOpen() && (e->modifiers() & Qt::ControlModifier))
    {
        QTextCursor cursor = cursorForPosition(mousePos);
        onLink = isOnLink(*document(), cursor, position_start, position_end, linkUrl);
    }

    if (onLink)
    {
        QTextEdit::ExtraSelection sel;
        sel.cursor = textCursor();
        sel.cursor.setPosition(position_start);
        sel.cursor.setPosition(position_end, QTextCursor::KeepAnchor);
        sel.format.setFontUnderline(true);
        linkSelections = { sel };
        viewport()->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        linkSelections.clear();
        viewport()->setCursor(Qt::IBeamCursor);
    }

    updateExtraSelections();
    QTextEdit::mouseMoveEvent(e);
}

void TextEdit::focusOutEvent(QFocusEvent *e)
{
    linkSelections.clear();
    updateExtraSelections();
    QTextEdit::focusOutEvent(e);
}

void TextEdit::fontChanged()
{
    setFont(settings.getNoteFont());
}

void TextEdit::linkOpenChanged()
{
    const bool is_link_open = settings.getNoteLinksOpen();
    setMouseTracking(is_link_open);
    if (!is_link_open)
    {
        linkSelections.clear();
        updateExtraSelections();
        viewport()->setCursor(Qt::IBeamCursor);
    }
}

bool TextEdit::findText(const QString &text, QTextDocument::FindFlags flags, bool wrap)
{
    if (text.isEmpty()) {
        return false;
    }

    // Attempt primary find from current position
    bool found = find(text, flags);

    // If not found and wrap is requested, restart from opposite edge
    if (!found && wrap) {
        QTextCursor originalCursor = textCursor();
        QTextCursor wrapCursor = originalCursor;

        if (flags.testFlag(QTextDocument::FindBackward)) {
            wrapCursor.movePosition(QTextCursor::End);
        } else {
            wrapCursor.movePosition(QTextCursor::Start);
        }
        setTextCursor(wrapCursor);

        found = find(text, flags);
        if (!found) {
            // Restore cursor if wrap search also fails
            setTextCursor(originalCursor);
        }
    }

    return found;
}

void TextEdit::highlightAll(const QString &text, QTextDocument::FindFlags flags)
{
    searchSelections.clear();

    if (!text.isEmpty()) {
        QTextDocument *doc = document();
        QTextCursor cursor(doc);

        QTextCharFormat highlightFormat;
        highlightFormat.setBackground(Qt::yellow);
        highlightFormat.setForeground(Qt::black);

        // Strip FindBackward for forward scanning
        flags &= ~QTextDocument::FindBackward;

        while (!(cursor = doc->find(text, cursor, flags)).isNull()) {
            QTextEdit::ExtraSelection extra;
            extra.format = highlightFormat;
            extra.cursor = cursor;
            searchSelections.append(extra);
        }
    }

    updateExtraSelections();
}
