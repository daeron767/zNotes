#include "highlighter.h"
#include "settings.h"
#include <QRegularExpression>
#include <QTextCharFormat>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!settings.getNoteHighlight())
    {
        setCurrentBlockState(0);
        return;
    }

    const auto &rules = settings.getHighlightRules();
    for (const HighlightRule &rule : rules)
    {
        if (!rule.enabled || !rule.regexp.isValid())
            continue;

        QRegularExpression expression(rule.regexp);
        if (!expression.isValid())
            continue;

        QTextCharFormat format;
        format.setForeground(rule.color);

        QRegularExpressionMatchIterator it = expression.globalMatch(text);
        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            int start = match.capturedStart();
            int length = match.capturedLength();

            if (length > 0)
            {
                setFormat(start, length, format);
            }
            else
            {
                // Prevent infinite loop on 0-length matches (e.g. boundary anchors)
                break;
            }
        }
    }

    setCurrentBlockState(0);
}
