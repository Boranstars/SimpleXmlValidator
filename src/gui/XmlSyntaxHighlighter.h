#ifndef SIMPLE_XML_VALIDATOR_GUI_XML_SYNTAX_HIGHLIGHTER_H
#define SIMPLE_XML_VALIDATOR_GUI_XML_SYNTAX_HIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class QTextDocument;

namespace simple_xml_validator::gui {

// 轻量 XML 语法高亮：标签、属性名、属性值、注释、实体引用着色。
// 仅用于只读展示（XML 预览与错误上下文），不改变文本内容。
class XmlSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit XmlSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    QVector<Rule>      rules_;
    QRegularExpression commentStart_;
    QRegularExpression commentEnd_;
    QTextCharFormat    commentFormat_;
};

}  // namespace simple_xml_validator::gui

#endif
