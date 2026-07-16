#include "gui/XmlSyntaxHighlighter.h"

namespace simple_xml_validator::gui {

XmlSyntaxHighlighter::XmlSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    // 标签尖括号与元素名（<tag ... > / </tag> / <tag/>）。
    QTextCharFormat tagFormat;
    tagFormat.setForeground(QColor("#2563eb"));
    tagFormat.setFontWeight(QFont::Bold);
    rules_.push_back({QRegularExpression(QStringLiteral("</?\\s*[A-Za-z_][\\w:.-]*")), tagFormat});

    QTextCharFormat bracketFormat;
    bracketFormat.setForeground(QColor("#2563eb"));
    rules_.push_back({QRegularExpression(QStringLiteral("/?>")), bracketFormat});

    // 属性名（name=）。
    QTextCharFormat attrFormat;
    attrFormat.setForeground(QColor("#b45309"));
    rules_.push_back({QRegularExpression(QStringLiteral("[A-Za-z_][\\w:.-]*(?=\\s*=)")), attrFormat});

    // 属性值（双引号或单引号字符串）。
    QTextCharFormat valueFormat;
    valueFormat.setForeground(QColor("#15803d"));
    rules_.push_back({QRegularExpression(QStringLiteral("\"[^\"]*\"|'[^']*'")), valueFormat});

    // 实体引用（&amp; &lt; 等）。
    QTextCharFormat entityFormat;
    entityFormat.setForeground(QColor("#9333ea"));
    rules_.push_back({QRegularExpression(QStringLiteral("&[A-Za-z#0-9]+;")), entityFormat});

    // 多行注释 <!-- ... -->。
    commentFormat_.setForeground(QColor("#6b7280"));
    commentFormat_.setFontItalic(true);
    commentStart_ = QRegularExpression(QStringLiteral("<!--"));
    commentEnd_   = QRegularExpression(QStringLiteral("-->"));
}

void XmlSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const Rule& rule : rules_) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 跨行注释处理：沿用 Qt 高亮器的 previousBlockState 机制。
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) {
        const QRegularExpressionMatch start = commentStart_.match(text);
        startIndex = start.hasMatch() ? start.capturedStart() : -1;
    }
    while (startIndex >= 0) {
        const QRegularExpressionMatch end = commentEnd_.match(text, startIndex);
        int commentLength = 0;
        if (end.hasMatch()) {
            commentLength = end.capturedEnd() - startIndex;
        } else {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        setFormat(startIndex, commentLength, commentFormat_);
        const QRegularExpressionMatch next = commentStart_.match(text, startIndex + commentLength);
        startIndex = next.hasMatch() ? next.capturedStart() : -1;
    }
}

}  // namespace simple_xml_validator::gui
