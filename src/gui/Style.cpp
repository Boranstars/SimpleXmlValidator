#include "gui/Style.h"

namespace simple_xml_validator::gui {

QString applicationStyleSheet() {
    // 浅色主题：浅灰底、白色面板、统一圆角与细边框。仅使用 QSS 能表达的属性，
    // 不引入任何素材；柔和投影、图标、语法高亮分别由代码/资源另行处理。
    return QStringLiteral(R"(
/* 整体背景与基础字体色 */
QMainWindow, QWidget#centralRoot {
    background-color: #f0f2f5;
}
QWidget {
    color: #1f2937;
    font-size: 10.5pt;
}

/* 菜单栏 */
QMenuBar {
    background-color: #ffffff;
    border-bottom: 1px solid #e5e7eb;
}
QMenuBar::item {
    padding: 6px 12px;
    background: transparent;
}
QMenuBar::item:selected {
    background-color: #eef2ff;
    color: #1d4ed8;
}
QMenu {
    background-color: #ffffff;
    border: 1px solid #e5e7eb;
}
QMenu::item:selected {
    background-color: #eef2ff;
    color: #1d4ed8;
}

/* 工具栏 */
QToolBar {
    background-color: #ffffff;
    border-bottom: 1px solid #e5e7eb;
    spacing: 6px;
    padding: 6px 8px;
}
QToolButton {
    padding: 6px 12px;
    border-radius: 6px;
    color: #374151;
}
QToolButton:hover {
    background-color: #f3f4f6;
}
QToolButton:pressed {
    background-color: #e5e7eb;
}
QToolButton:disabled {
    color: #9ca3af;
}

/* 分组框 */
QGroupBox {
    background-color: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    margin-top: 10px;
    font-weight: 600;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 4px;
    color: #374151;
}

/* 输入框 */
QLineEdit {
    background-color: #ffffff;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    padding: 6px 8px;
    selection-background-color: #bfdbfe;
}
QLineEdit:focus {
    border: 1px solid #2563eb;
}
QLineEdit:read-only {
    background-color: #f9fafb;
}

/* 普通/次级按钮 */
QPushButton {
    background-color: #ffffff;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    padding: 6px 14px;
    color: #374151;
}
QPushButton:hover {
    background-color: #f3f4f6;
}
QPushButton:pressed {
    background-color: #e5e7eb;
}
QPushButton:disabled {
    color: #9ca3af;
    border-color: #e5e7eb;
}

/* 主操作按钮（开始校验） */
QPushButton#primaryButton {
    background-color: #2563eb;
    border: none;
    border-radius: 6px;
    padding: 8px 20px;
    color: #ffffff;
    font-weight: bold;
}
QPushButton#primaryButton:hover {
    background-color: #1d4ed8;
}
QPushButton#primaryButton:pressed {
    background-color: #1e40af;
}
QPushButton#primaryButton:disabled {
    background-color: #93b4f5;
    color: #eef2ff;
}

/* 状态卡片 */
QFrame#card {
    background-color: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 10px;
}

/* 标签页 */
QTabWidget::pane {
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    top: -1px;
    background-color: #ffffff;
}
QTabBar::tab {
    background: transparent;
    padding: 8px 16px;
    margin-right: 4px;
    color: #6b7280;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    color: #2563eb;
    border-bottom: 2px solid #2563eb;
    font-weight: 600;
}
QTabBar::tab:hover {
    color: #2563eb;
}

/* 表格 */
QTableWidget {
    background-color: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    gridline-color: #eef0f3;
    selection-background-color: #e8f0fe;
    selection-color: #1f2937;
}
QTableWidget::item {
    padding: 4px 6px;
}
QHeaderView::section {
    background-color: #f7f8fa;
    padding: 6px 8px;
    border: none;
    border-bottom: 1px solid #e5e7eb;
    color: #4b5563;
    font-weight: 600;
}

/* 文件列表 */
QListWidget {
    background-color: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
}
QListWidget::item {
    padding: 8px 10px;
    border-radius: 6px;
}
QListWidget::item:selected {
    background-color: #e8f0fe;
    color: #1d4ed8;
}
QListWidget::item:hover {
    background-color: #f3f4f6;
}

/* 代码/文本区域 */
QPlainTextEdit {
    background-color: #fbfcfd;
    border: 1px solid #e5e7eb;
    border-radius: 6px;
    font-family: "Consolas", "DejaVu Sans Mono", "Courier New", monospace;
    selection-background-color: #bfdbfe;
}

/* 状态栏 */
QStatusBar {
    background-color: #ffffff;
    border-top: 1px solid #e5e7eb;
    color: #6b7280;
}

/* 分隔条 */
QSplitter::handle {
    background-color: transparent;
}
)");
}

}  // namespace simple_xml_validator::gui
