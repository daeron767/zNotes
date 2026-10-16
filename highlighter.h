#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit Highlighter(QTextDocument *parent = nullptr);
    ~Highlighter() override = default;

protected:
    void highlightBlock(const QString &text) override;
};

#endif // HIGHLIGHTER_H
