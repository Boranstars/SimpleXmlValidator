#include "ValidationLog.h"

#include <string>

namespace simple_xml_validator::infrastructure::logging {
namespace {

// 以 UTF-8 表达路径，保证 Windows/Linux 下中文与空格路径记录一致。
std::string pathToUtf8(const std::filesystem::path& path) {
    return path.u8string();
}

const char* statusText(ValidationStatus status) noexcept {
    switch (status) {
        case ValidationStatus::Valid:
            return "Valid";
        case ValidationStatus::Invalid:
            return "Invalid";
        case ValidationStatus::Failed:
            return "Failed";
    }
    return "Unknown";
}

const char* severityText(ErrorSeverity severity) noexcept {
    switch (severity) {
        case ErrorSeverity::Warning:
            return "Warning";
        case ErrorSeverity::Error:
            return "Error";
        case ErrorSeverity::Fatal:
            return "Fatal";
    }
    return "Unknown";
}

}  // namespace

void logValidationRecord(const LogModule&             module,
                         const std::filesystem::path& xmlPath,
                         const std::filesystem::path& xsdPath,
                         const ValidationResult&      result) noexcept {
    if (!module.ready()) {
        return;
    }
    // 记录构建（u8string、字符串拼接、to_string）均可能抛出；本函数为 noexcept，
    // 必须在此吞掉全部异常，保证格式化失败不会终止程序或影响调用方主流程。
    try {
        std::string record = "XML 校验记录";
        record += "\n  XML 文件: " + pathToUtf8(xmlPath);
        record += "\n  XSD 文件: " + pathToUtf8(xsdPath);
        record += "\n  校验状态: " + std::string(statusText(result.status));
        record += "\n  错误数量: " + std::to_string(result.errors.size());
        if (!result.message.empty()) {
            record += "\n  说明: " + result.message;
        }
        for (const ValidationError& error : result.errors) {
            record += "\n  - [" + std::string(severityText(error.severity)) + "] 行 " +
                      std::to_string(error.line) + " 列 " +
                      std::to_string(error.column) + " : " + error.message;
        }
        module.info(record);
    } catch (...) {
        // 校验记录格式化或写入失败不得影响校验主流程。
    }
}

}  // namespace simple_xml_validator::infrastructure::logging
