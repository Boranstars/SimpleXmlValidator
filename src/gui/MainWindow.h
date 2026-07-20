#ifndef SIMPLE_XML_VALIDATOR_GUI_MAIN_WINDOW_H
#define SIMPLE_XML_VALIDATOR_GUI_MAIN_WINDOW_H

#include "core/validation/XmlValidator.h"
#include "gui/ValidationResultPresenter.h"
#include "infrastructure/logging/Logger.h"

#include <QMainWindow>

#include <filesystem>

class QAction;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QObject;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;

namespace simple_xml_validator::infrastructure::xerces {
class XercesRuntime;
}

namespace simple_xml_validator::gui {

class CodeEditor;
class XmlSyntaxHighlighter;

// 主窗口：对齐 docs/ui_raw/概念设计图.jpg 的最小实现——工具栏、文件列表、
// 输入配置、状态卡片（状态/错误数/警告数/耗时）、结果多标签（校验结果/XML 预览/
// 错误日志/统计信息）与错误详情面板。GUI 不直接包含或调用 Xerces/spdlog；
// 校验交由 XmlValidator，日志经 LogModule。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        const infrastructure::xerces::XercesRuntime& runtime,
        infrastructure::logging::LogManager*         logManager = nullptr,
        QWidget*                                     parent     = nullptr);
    ~MainWindow() override;

protected:
    // 通过事件过滤器为 XML/XSD 各自的路径框处理拖入与放下事件。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();

    void onSelectXml();
    void onSelectXsd();
    void onValidate();
    void onReset();
    void onErrorSelectionChanged();
    void onPrevPage();
    void onNextPage();
    void onPageSizeChanged();

    void setXmlPath(const std::filesystem::path& path);
    void setXsdPath(const std::filesystem::path& path);
    void updateValidateButtonState();
    void clearResultArea();
    void applyPresented(const PresentedResult& presented, double elapsedMilliseconds);
    void refreshFileList();
    void loadXmlPreview();
    void renderErrorPage();
    void updateErrorDetail(int absoluteIndex);

    infrastructure::logging::LogModule guiLog_;
    XmlValidator                       validator_;
    ValidationResultPresenter          presenter_;

    std::filesystem::path xmlPath_;
    std::filesystem::path xsdPath_;
    PresentedResult       presented_;  // 最近一次展示状态，供错误详情/上下文引用

    // 工具栏动作。
    QAction* validateAction_ = nullptr;

    // 输入配置。
    QLineEdit*   xmlPathEdit_     = nullptr;
    QLineEdit*   xsdPathEdit_     = nullptr;
    QPushButton* selectXmlButton_ = nullptr;
    QPushButton* selectXsdButton_ = nullptr;
    QPushButton* validateButton_  = nullptr;
    QPushButton* resetButton_     = nullptr;

    // 左侧文件列表。
    QListWidget* fileList_ = nullptr;

    // 状态卡片值标签。
    QLabel* statusValueLabel_       = nullptr;
    QLabel* errorCountValueLabel_   = nullptr;
    QLabel* warningCountValueLabel_ = nullptr;
    QLabel* elapsedValueLabel_      = nullptr;

    // 结果区提示条与多标签。
    QLabel*       bannerLabel_     = nullptr;
    QLabel*       errorCountLabel_ = nullptr;
    QTableWidget* errorTable_      = nullptr;
    QComboBox*    pageSizeCombo_   = nullptr;
    QPushButton*  prevPageButton_  = nullptr;
    QPushButton*  nextPageButton_  = nullptr;
    QLabel*       pageInfoLabel_   = nullptr;
    QTabWidget*   resultTabs_      = nullptr;
    CodeEditor*   xmlPreview_      = nullptr;
    QPlainTextEdit* logView_       = nullptr;
    QLabel*       statsLabel_      = nullptr;

    int currentPage_ = 0;
    int pageSize_    = 100;

    // 右侧错误详情面板。
    QLabel*     detailLevelLabel_   = nullptr;
    QLabel*     detailFileLabel_    = nullptr;
    QLabel*     detailLineLabel_    = nullptr;
    QLabel*     detailColumnLabel_  = nullptr;
    QPlainTextEdit* detailMessageEdit_ = nullptr;
    CodeEditor* contextEdit_        = nullptr;

    XmlSyntaxHighlighter* previewHighlighter_ = nullptr;
    XmlSyntaxHighlighter* contextHighlighter_ = nullptr;
};

}  // namespace simple_xml_validator::gui

#endif
