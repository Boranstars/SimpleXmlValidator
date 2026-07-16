#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_LOGGING_VALIDATION_LOG_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_LOGGING_VALIDATION_LOG_H

#include "core/validation/ValidationResult.h"
#include "infrastructure/logging/Logger.h"

#include <filesystem>

namespace simple_xml_validator::infrastructure::logging {

// 领域记录 helper（通用日志机制之上的薄封装）：在 XML 校验记录通道对应的
// 模块句柄上写入一条包含完整 ValidationResult 明细的记录，保证字段完整、
// 格式统一。module 应为调用方从 LogChannel::Validation 申请的句柄；句柄为
// 空操作时静默降级。
void logValidationRecord(const LogModule&             module,
                         const std::filesystem::path& xmlPath,
                         const std::filesystem::path& xsdPath,
                         const ValidationResult&      result) noexcept;

}  // namespace simple_xml_validator::infrastructure::logging

#endif
