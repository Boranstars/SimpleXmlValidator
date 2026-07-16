#include "gui/CodeEditor.h"

#include <QColor>
#include <QPainter>
#include <QRect>
#include <QResizeEvent>
#include <QSize>
#include <QTextBlock>
#include <QTextEdit>

#include <algorithm>

namespace simple_xml_validator::gui {

LineNumberArea::LineNumberArea(CodeEditor* editor)
    : QWidget(editor), editor_(editor) {}

QSize LineNumberArea::sizeHint() const {
    return QSize(editor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
    editor_->lineNumberAreaPaintEvent(event);
}

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
    lineNumberArea_ = new LineNumberArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);

    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    updateLineNumberAreaWidth(0);
}

int CodeEditor::lineNumberAreaWidth() const {
    // 依据最大行号（考虑起始偏移）估算所需宽度。
    int maxLine  = std::max(1, firstLineNumber_ + std::max(0, blockCount() - 1));
    int digits   = 1;
    while (maxLine >= 10) {
        maxLine /= 10;
        ++digits;
    }
    const int padding = 12;
    return padding + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + padding;
}

void CodeEditor::setFirstLineNumber(int lineNumber) {
    firstLineNumber_ = std::max(1, lineNumber);
    updateLineNumberAreaWidth(0);
    lineNumberArea_->update();
}

void CodeEditor::setHighlightLine(int absoluteLineNumber) {
    highlightLine_ = absoluteLineNumber;
    refreshExtraSelections();
    lineNumberArea_->update();
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy != 0) {
        lineNumberArea_->scroll(0, dy);
    } else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::refreshExtraSelections() {
    QList<QTextEdit::ExtraSelection> selections;
    if (highlightLine_ > 0) {
        const int blockIndex = highlightLine_ - firstLineNumber_;
        if (blockIndex >= 0 && blockIndex < blockCount()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(QColor("#fff3cd"));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor cursor(document()->findBlockByNumber(blockIndex));
            selection.cursor = cursor;
            selection.cursor.clearSelection();
            selections.append(selection);
        }
    }
    setExtraSelections(selections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor("#f3f4f6"));

    QTextBlock block       = firstVisibleBlock();
    int        blockNumber = block.blockNumber();
    int top    = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int     lineNumber = blockNumber + firstLineNumber_;
            const bool    active     = (lineNumber == highlightLine_);
            const QString number     = QString::number(lineNumber);
            painter.setPen(active ? QColor("#c62828") : QColor("#9ca3af"));
            painter.drawText(0, top, lineNumberArea_->width() - 8,
                             fontMetrics().height(), Qt::AlignRight, number);
        }
        block  = block.next();
        top    = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

}  // namespace simple_xml_validator::gui
