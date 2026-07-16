#ifndef SIMPLE_XML_VALIDATOR_GUI_MAIN_WINDOW_H
#define SIMPLE_XML_VALIDATOR_GUI_MAIN_WINDOW_H

#include "core/validation/XmlValidator.h"
#include "gui/ValidationResultPresenter.h"
#include "infrastructure/logging/Logger.h"

#include <QMainWindow>

#include <filesystem>

class QEvent;
class QLabel;
class QLineEdit;
class QObject;
class QPushButton;
class QTableWidget;

namespace simple_xml_validator::infrastructure::xerces {
class XercesRuntime;
}

namespace simple_xml_validator::gui {

// 主窗口：XML/XSD 文件选择与独立拖放、同步校验、结果展示与阻断性弹窗。
// GUI 不直接包含或调用 Xerces/spdlog；校验交由 XmlValidator，日志经 LogModule。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        const infrastructure::xerces::XercesRuntime& runtime,
        infrastructure::logging::LogManager*         logManager = nullptr,
        QWidget*                                     parent     = nullptr);
    ~MainWindow() override;

protected:
    // 通过事件过滤器为 XML/XSD 各自的拖放区域处理拖入与放下事件。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();

    void onSelectXml();
    void onSelectXsd();
    void onValidate();
    void onReset();

    void setXmlPath(const std::filesystem::path& path);
    void setXsdPath(const std::filesystem::path& path);
    void updateValidateButtonState();
    void clearResultArea();
    void applyPresented(const PresentedResult& presented);

    infrastructure::logging::LogModule guiLog_;
    XmlValidator                       validator_;
    ValidationResultPresenter          presenter_;

    std::filesystem::path xmlPath_;
    std::filesystem::path xsdPath_;

    QLineEdit*    xmlPathEdit_     = nullptr;
    QLineEdit*    xsdPathEdit_     = nullptr;
    QPushButton*  selectXmlButton_ = nullptr;
    QPushButton*  selectXsdButton_ = nullptr;
    QPushButton*  validateButton_  = nullptr;
    QPushButton*  resetButton_     = nullptr;
    QLabel*       bannerLabel_     = nullptr;
    QLabel*       errorCountLabel_ = nullptr;
    QTableWidget* errorTable_      = nullptr;
};

}  // namespace simple_xml_validator::gui

#endif
