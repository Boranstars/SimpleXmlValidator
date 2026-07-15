#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_SCHEMA_VALIDATOR_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_SCHEMA_VALIDATOR_H

#include "core/validation/ValidationResult.h"

#include <filesystem>
#include <string>
#include <vector>

namespace simple_xml_validator::infrastructure::xerces {

// 区分“Schema/引擎阻断失败”与“XML 校验已完成但可能存在诊断”。
// 最终 ValidationStatus 映射由 XmlValidator 负责。
enum class SchemaValidationStage {
    Blocked,   // XSD 语法/依赖/加载失败或引擎阻断异常
    Completed  // XML 已完成解析与 Schema 校验（errors 可能非空）
};

struct SchemaValidationReport {
    SchemaValidationStage        stage;
    std::vector<ValidationError> errors;   // 仅在 Completed 时填充 XML 内容诊断
    std::string                  message;  // Blocked 原因或校验摘要
};

// 封装 Xerces 解析器配置、XSD Grammar 预加载、XML 解析与 Schema 校验。
// 不向 core 或 GUI 泄漏 Xerces 原始类型。调用前必须确保 Xerces 已初始化。
class XercesSchemaValidator {
public:
    SchemaValidationReport validate(
        const std::filesystem::path& xmlPath,
        const std::filesystem::path& xsdPath);
};

}

#endif
