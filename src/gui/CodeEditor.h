#ifndef SIMPLE_XML_VALIDATOR_GUI_CODE_EDITOR_H
#define SIMPLE_XML_VALIDATOR_GUI_CODE_EDITOR_H

#include <QPlainTextEdit>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

namespace simple_xml_validator::gui {

// 带行号栏与「高亮某行」能力的只读代码视图，用于 XML 预览与错误上下文。
// 沿用 Qt 官方 Code Editor 示例结构，另加：起始行号偏移（上下文可从真实行号起排）
// 与按绝对行号高亮当前出错行。
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int  lineNumberAreaWidth() const;

    // 行号栏显示的第一行编号（默认为 1）。上下文视图可设为窗口起始的真实行号。
    void setFirstLineNumber(int lineNumber);
    // 以绝对行号高亮某行；<=0 表示不高亮。
    void setHighlightLine(int absoluteLineNumber);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void refreshExtraSelections();

private:
    QWidget* lineNumberArea_;
    int      firstLineNumber_ = 1;
    int      highlightLine_   = -1;
};

// 行号栏子控件：把绘制转交给 CodeEditor。
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CodeEditor* editor_;
};

}  // namespace simple_xml_validator::gui

#endif
