#include "XmlValidator.h"

#include "core/validation/InputValidator.h"
#include "infrastructure/xerces/XercesRuntime.h"
#include "infrastructure/xerces/XercesSchemaValidator.h"

namespace xerces_ns = simple_xml_validator::infrastructure::xerces;

XmlValidator::XmlValidator(const xerces_ns::XercesRuntime& runtime)
    : runtime_(&runtime) {}

ValidationResult XmlValidator::validate(
    const std::filesystem::path& xmlPath,
    const std::filesystem::path& xsdPath) {
    const InputCheckResult xmlCheck = InputValidator::check(xmlPath);
    if (!xmlCheck.ok) {
        return {ValidationStatus::Failed, {}, "XML 输入检查失败：" + xmlCheck.errorMessage};
    }

    const InputCheckResult xsdCheck = InputValidator::check(xsdPath);
    if (!xsdCheck.ok) {
        return {ValidationStatus::Failed, {}, "XSD 输入检查失败：" + xsdCheck.errorMessage};
    }

    if (runtime_ == nullptr || !runtime_->isReady()) {
        std::string reason = runtime_ ? runtime_->errorMessage() : "运行时不可用";
        return {ValidationStatus::Failed, {}, "XML 校验引擎初始化失败：" + reason};
    }

    xerces_ns::XercesSchemaValidator schemaValidator;
    xerces_ns::SchemaValidationReport report =
        schemaValidator.validate(xmlCheck.absolutePath, xsdCheck.absolutePath);

    if (report.stage == xerces_ns::SchemaValidationStage::Blocked) {
        return {ValidationStatus::Failed, {}, report.message};
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
