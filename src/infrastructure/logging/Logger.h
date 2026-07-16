#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_LOGGING_LOGGER_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_LOGGING_LOGGER_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

// 前置声明 spdlog 类型，避免在项目头文件中泄漏 spdlog 原始类型。
namespace spdlog {
class logger;
namespace sinks {
class sink;
}
}  // namespace spdlog

namespace simple_xml_validator::infrastructure::logging {

// 模块日志级别。
enum class LogLevel {
    Info,
    Warning,
    Error
};

// 逻辑日志通道。两个通道在物理上默认写入不同文件，语义上分别对应
// 系统运行日志与 XML 校验记录（详见概要设计 §4.6）。
enum class LogChannel {
    System,      // 系统运行日志
    Validation   // XML 校验记录
};

// 日志运行环境配置。
struct LogConfig {
    std::filesystem::path directory;
    std::string systemLogFileName     = "system_runtime.log";
    std::string validationLogFileName = "xml_validation.log";

    // 日志滚动（rotating）配置。默认关闭，保持单文件追加行为，
    // 与旧版语义一致。开启后单个文件达到上限即滚动，历史文件轮转保留。
    bool        rotating         = false;
    // 单个日志文件的最大字节数；达到后滚动到下一个文件。
    std::size_t maxFileSizeBytes = 5 * 1024 * 1024;  // 5 MiB
    // 保留的历史滚动文件数量（不含当前正在写入的文件）。
    std::size_t maxFiles         = 3;
};

// 绑定「模块名」的日志句柄，输出统一格式 [时间][模块][级别][内容]。
// 由 LogManager 创建；默认构造得到一个空操作句柄（任何写入均被忽略）。
// 可自由拷贝，成本低。任何写入异常都在内部吞掉，不影响调用方。
class LogModule {
public:
    LogModule() noexcept = default;

    // 入口参数使用 string_view：从字面量/已有字符串构造它不分配、不抛异常，
    // 调用方不会因「仅为写日志」的形参构造失败而崩溃或改变主流程；任何必要的
    // 字符串复制与 spdlog 调用均在 .cpp 的异常边界内完成。
    void log(LogLevel level, std::string_view message) const noexcept;
    void info(std::string_view message) const noexcept;
    void warning(std::string_view message) const noexcept;
    void error(std::string_view message) const noexcept;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& name() const noexcept;

private:
    friend class LogManager;
    LogModule(std::shared_ptr<spdlog::logger> logger, std::string name) noexcept;

    std::shared_ptr<spdlog::logger> logger_;
    std::string                     name_;
};

// 通用模块化日志器。作为组合根持有底层 sink，向各功能模块发放 LogModule。
// 不依赖 Qt，不向调用方暴露 spdlog 类型。任一通道创建失败都会降级为不可用，
// 对应通道发放的 LogModule 为空操作句柄。
class LogManager final {
public:
    explicit LogManager(const LogConfig& config) noexcept;
    ~LogManager();

    LogManager(const LogManager&)            = delete;
    LogManager& operator=(const LogManager&) = delete;

    // 申请一个绑定模块名、写入指定通道的日志句柄。name 使用 string_view，
    // 形参构造不分配、不抛异常；内部所需的字符串复制在 .cpp 的异常边界内完成。
    [[nodiscard]] LogModule module(
        std::string_view name, LogChannel channel = LogChannel::System) noexcept;

    [[nodiscard]] bool channelReady(LogChannel channel) const noexcept;

private:
    std::shared_ptr<spdlog::sinks::sink> systemSink_;
    std::shared_ptr<spdlog::sinks::sink> validationSink_;
};

}  // namespace simple_xml_validator::infrastructure::logging

#endif
