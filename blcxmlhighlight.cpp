#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif
#include "inc/blcxmlhighlight.h"
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>


BlcXmlHighlight::BlcXmlHighlight(QTextDocument* parent):
    QSyntaxHighlighter(parent)
{

}

void BlcXmlHighlight::highlightBlock(const QString &text)
{
    QTextCharFormat blueFormat;
    blueFormat.setForeground(Qt::blue);

    QTextCharFormat redFormat;
    redFormat.setForeground(Qt::red);

    QTextCharFormat strFormat;
    strFormat.setForeground(Qt::magenta);

    QRegularExpression tagExpression("\\</?\\w{1,50}\\>");
    QRegularExpressionMatchIterator i = tagExpression.globalMatch(text);
    while (i.hasNext())
    {
      QRegularExpressionMatch match = i.next();
      setFormat(match.capturedStart(), match.capturedLength(), blueFormat);
    }

    QRegularExpression strExpression("'(\\\\.|[^'])*'");
    QRegularExpressionMatchIterator k = strExpression.globalMatch(text);
    while(k.hasNext())
    {
        QRegularExpressionMatch match = k.next();
        setFormat(match.capturedStart(), match.capturedLength(), strFormat);
    }

    QRegularExpression xmlInfoExpression("(\\<\\?)(.*)(xml)(.*)(\\bversion\\b)(.*)(\\bencoding\\b)(.*)(\\?\\>)");//匹配 <?xml version='1.0' encoding='utf-8'?>
    QRegularExpressionMatch m = xmlInfoExpression.match(text);
    if(m.hasMatch())
    {
        setFormat(m.capturedStart(1), m.capturedLength(1), redFormat);
        setFormat(m.capturedStart(3), m.capturedLength(3), blueFormat);
        setFormat(m.capturedStart(5), m.capturedLength(5), redFormat);
        setFormat(m.capturedStart(7), m.capturedLength(7), redFormat);
        setFormat(m.capturedStart(9), m.capturedLength(9), redFormat);
    }
}
