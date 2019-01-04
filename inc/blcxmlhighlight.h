#ifndef BLCXMLHIGHLIGHT_H
#define BLCXMLHIGHLIGHT_H

#include <QSyntaxHighlighter>
#include <QTextDocument>

class BlcXmlHighlight : public QSyntaxHighlighter
{
public:
    explicit BlcXmlHighlight(QTextDocument* parent);
protected:
    void highlightBlock(const QString &text);
};

#endif // BLCXMLHIGHLIGHT_H
