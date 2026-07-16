#include "XmlValidator.h"

#include "core/validation/InputValidator.h"
#include "infrastructure/logging/Logger.h"
#include "infrastructure/logging/ValidationLog.h"
#include "infrastructure/xerces/XercesRuntime.h"
#include "infrastructure/xerces/XercesSchemaValidator.h"

#include <exception>
#include <string>

namespace xerces_ns  = simple_xml_validator::infrastructure::xerces;
namespace logging_ns = simple_xml_validator::infrastructure::logging;

namespace {

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

// 系统面包屑：开始校验。日志专属字符串的构建被包在异常边界内，
// 确保日志格式化/分配失败绝不改变或中断校验结果。
void logValidationStart(const logging_ns::LogModule&  systemLog,
                        const std::filesystem::path& xmlPath,
                        const std::filesystem::path& xsdPath) noexcept {
    try {
        systemLog.info("开始校验 XML=" + xmlPath.u8string() +
                       " XSD=" + xsdPath.u8string());
    } catch (...) {
    }
}

// 系统面包屑：校验结束状态摘要（完整明细进 XML 校验日志，不在此重复）。
void logValidationFinish(const logging_ns::LogModule& systemLog,
                         const ValidationResult&      result) noexcept {
    try {
        systemLog.info(std::string("校验结束，状态=") + statusText(result.status) +
                       "，错误数量=" + std::to_string(result.errors.size()));
    } catch (...) {
    }
}

}  // namespace

XmlValidator::XmlValidator(const xerces_ns::XercesRuntime& runtime,
                           logging_ns::LogManager*         logManager)
    : runtime_(&runtime), logManager_(logManager) {}

ValidationResult XmlValidator::validate(
    const std::filesystem::path& xmlPath,
    const std::filesystem::path& xsdPath) {
    // 作为一个功能模块，向日志器申请自身的系统日志句柄与校验记录句柄；
    // logManager_ 为空时得到空操作句柄，全流程静默降级。
    const logging_ns::LogModule systemLog =
        logManager_ ? logManager_->module("XmlValidator", logging_ns::LogChannel::System)
                    : logging_ns::LogModule{};
    const logging_ns::LogModule recordLog =
        logManager_ ? logManager_->module("ValidationRecord", logging_ns::LogChannel::Validation)
                    : logging_ns::LogModule{};

    logValidationStart(systemLog, xmlPath, xsdPath);

    ValidationResult result;
    try {
        result = runValidation(xmlPath, xsdPath, systemLog);
    } catch (const std::exception& e) {
        result = {ValidationStatus::Failed, {},
                  std::string("校验过程发生未预期异常：") + e.what()};
        systemLog.error(result.message);
    } catch (...) {
        result = {ValidationStatus::Failed, {}, "校验过程发生未预期异常"};
        systemLog.error(result.message);
    }

    // 系统面包屑记录结束状态；XML 校验记录写入完整结果。二者句柄不可用时静默降级。
    logValidationFinish(systemLog, result);
    logging_ns::logValidationRecord(recordLog, xmlPath, xsdPath, result);
    return result;
}

ValidationResult XmlValidator::runValidation(
    const std::filesystem::path&   xmlPath,
    const std::filesystem::path&   xsdPath,
    const logging_ns::LogModule&   systemLog) {
    // 各失败分支均先构造 ValidationResult，再按引用把已有的结果消息交给系统日志；
    // 调用点不再为日志单独拼接字符串，日志失败不会改变返回的校验结果。
    const InputCheckResult xmlCheck = InputValidator::check(xmlPath);
    if (!xmlCheck.ok) {
        ValidationResult result{ValidationStatus::Failed, {},
                                "XML 输入检查失败：" + xmlCheck.errorMessage};
        systemLog.warning(result.message);
        return result;
    }

    const InputCheckResult xsdCheck = InputValidator::check(xsdPath);
    if (!xsdCheck.ok) {
        ValidationResult result{ValidationStatus::Failed, {},
                                "XSD 输入检查失败：" + xsdCheck.errorMessage};
        systemLog.warning(result.message);
        return result;
    }

    if (runtime_ == nullptr || !runtime_->isReady()) {
        std::string reason = runtime_ ? runtime_->errorMessage() : "运行时不可用";
        ValidationResult result{ValidationStatus::Failed, {},
                                "XML 校验引擎初始化失败：" + reason};
        systemLog.error(result.message);
        return result;
    }

    xerces_ns::XercesSchemaValidator schemaValidator;
    xerces_ns::SchemaValidationReport report =
        schemaValidator.validate(xmlCheck.absolutePath, xsdCheck.absolutePath);

    if (report.stage == xerces_ns::SchemaValidationStage::Blocked) {
        ValidationResult result{ValidationStatus::Failed, {}, report.message};
        systemLog.error(result.message);
        return result;
    }

    std::size_t blockingCount = 0;
    for (const ValidationError& error : report.errors) {
        if (error.severity != ErrorSeverity::Warning) {
            ++blockingCount;
        }
    }

    if (blockingCount > 0) {
        ValidationResult result;
        result.status  = ValidationStatus::Invalid;
        result.errors  = std::move(report.errors);
        result.message = "XML 未通过校验，共 " + std::to_string(blockingCount) + " 处问题";
        return result;
    }

    ValidationResult result;
    result.status  = ValidationStatus::Valid;
    result.errors  = std::move(report.errors);
    result.message = "XML 通过校验";
    return result;
}
