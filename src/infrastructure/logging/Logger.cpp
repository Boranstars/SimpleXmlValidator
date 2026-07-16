#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <system_error>
#include <utility>

namespace simple_xml_validator::infrastructure::logging {
namespace {

// 创建通道文件 sink。目录创建或文件打开失败在此降级为 nullptr，不抛异常。
// 依据配置选择单文件追加 sink 或滚动 sink，滚动参数经过下限保护。
std::shared_ptr<spdlog::sinks::sink> makeFileSink(
    const LogConfig&   config,
    const std::string& fileName) noexcept {
    try {
        if (!config.directory.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(config.directory, ec);
            // 忽略 ec：若目录不可用，随后打开文件会失败并在下方降级。
        }
        const std::filesystem::path filePath =
            config.directory.empty() ? std::filesystem::path(fileName)
                                     : config.directory / fileName;
        if (config.rotating) {
            // 保证滚动参数有效：单文件至少 1 字节、至少保留 1 个历史文件，
            // 避免 spdlog 因 0 值抛出异常而导致整个通道不可用。
            const std::size_t maxFileSize = std::max<std::size_t>(config.maxFileSizeBytes, 1);
            const std::size_t maxFiles    = std::max<std::size_t>(config.maxFiles, 1);
            return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filePath.string(), maxFileSize, maxFiles);
        }
        return std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            filePath.string(), false);
    } catch (...) {
        return nullptr;
    }
}

}  // namespace

LogModule::LogModule(std::shared_ptr<spdlog::logger> logger, std::string name) noexcept
    : logger_(std::move(logger)), name_(std::move(name)) {}

void LogModule::log(LogLevel level, std::string_view message) const noexcept {
    if (!logger_) {
        return;
    }
    try {
        // spdlog 的 string_view_t 重载按原样记录（不做 {} 格式解析）；同步 sink
        // 在调用返回前即消费内容，不存在悬垂。任何内部分配失败在此吞掉。
        const spdlog::string_view_t view{message.data(), message.size()};
        switch (level) {
            case LogLevel::Info:
                logger_->info(view);
                break;
            case LogLevel::Warning:
                logger_->warn(view);
                break;
            case LogLevel::Error:
                logger_->error(view);
                break;
        }
    } catch (...) {
        // 写入失败不得影响调用方主流程。
    }
}

void LogModule::info(std::string_view message) const noexcept {
    log(LogLevel::Info, message);
}

void LogModule::warning(std::string_view message) const noexcept {
    log(LogLevel::Warning, message);
}

void LogModule::error(std::string_view message) const noexcept {
    log(LogLevel::Error, message);
}

bool LogModule::ready() const noexcept {
    return static_cast<bool>(logger_);
}

const std::string& LogModule::name() const noexcept {
    return name_;
}

LogManager::LogManager(const LogConfig& config) noexcept {
    systemSink_     = makeFileSink(config, config.systemLogFileName);
    validationSink_ = makeFileSink(config, config.validationLogFileName);
}

LogManager::~LogManager() = default;

LogModule LogManager::module(std::string_view name, LogChannel channel) noexcept {
    const std::shared_ptr<spdlog::sinks::sink>& sink =
        (channel == LogChannel::Validation) ? validationSink_ : systemSink_;
    if (!sink) {
        return LogModule{};
    }
    try {
        // name 的复制（spdlog logger 名与 LogModule::name_）在异常边界内完成；
        // 分配失败降级为空操作句柄，不抛异常、不改变调用方主流程。
        std::string moduleName{name};
        // 不注册到 spdlog 全局注册表，避免多实例/多模块名称冲突；
        // 同一通道的多个模块共享同一个文件 sink。
        auto logger = std::make_shared<spdlog::logger>(moduleName, sink);
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
        // 输出格式：[时间][模块][级别][内容]，其中 %n 为 logger 名（即模块名）。
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%n][%l] %v");
        return LogModule{std::move(logger), std::move(moduleName)};
    } catch (...) {
        return LogModule{};
    }
}

bool LogManager::channelReady(LogChannel channel) const noexcept {
    return static_cast<bool>(
        (channel == LogChannel::Validation) ? validationSink_ : systemSink_);
}

}  // namespace simple_xml_validator::infrastructure::logging
