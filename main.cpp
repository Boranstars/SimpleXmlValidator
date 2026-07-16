#include "gui/MainWindow.h"
#include "gui/Style.h"
#include "infrastructure/logging/Logger.h"
#include "infrastructure/xerces/XercesRuntime.h"

#include <QApplication>

#include <filesystem>
#include <string>

namespace logging_ns = simple_xml_validator::infrastructure::logging;
namespace xerces_ns  = simple_xml_validator::infrastructure::xerces;
namespace gui_ns     = simple_xml_validator::gui;

int main(int argc, char *argv[]) {
    // 组合根：先建立日志运行环境，并在整个应用生命周期内保持可用。
    logging_ns::LogConfig logConfig;
    logConfig.directory = std::filesystem::path("logs");

    logging_ns::LogManager logManager(logConfig);

    // 各功能模块按需申请绑定模块名的日志句柄。
    const logging_ns::LogModule appLog = logManager.module("App");
    appLog.info("程序启动");

    // Xerces 运行时作为组合根持有，生命周期覆盖后续校验器与解析器。
    xerces_ns::XercesRuntime runtime;
    const logging_ns::LogModule xercesLog = logManager.module("Xerces");
    if (runtime.isReady()) {
        xercesLog.info("Xerces 运行时初始化成功");
    } else {
        xercesLog.error("Xerces 运行时初始化失败：" + runtime.errorMessage());
    }

    QApplication a(argc, argv);
    a.setStyleSheet(gui_ns::applicationStyleSheet());
    gui_ns::MainWindow window(runtime, &logManager);
    window.resize(1180, 720);
    window.show();
    const int code = QApplication::exec();

    // 组合根作用域即将结束，XercesRuntime 随后析构并释放 Xerces。
    xercesLog.info("Xerces 运行时即将释放");
    appLog.info("程序退出，返回码 " + std::to_string(code));
    return code;
}
