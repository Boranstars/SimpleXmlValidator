#ifndef SIMPLE_XML_VALIDATOR_CORE_VALIDATION_VALIDATION_RESULT_H
#define SIMPLE_XML_VALIDATOR_CORE_VALIDATION_VALIDATION_RESULT_H

#include <cstddef>
#include <string>
#include <vector>

enum class ValidationStatus {
    Valid,    // XML 格式良好且通过 XSD 校验
    Invalid,  // 校验流程完成，但 XML 不符合 XSD/格式要求
    Failed    // 前置检查、XSD 加载或程序异常导致校验无法完成
};

enum class ErrorSeverity {
    Warning,
    Error,
    Fatal
};

struct ValidationError {
    ErrorSeverity severity;
    std::size_t   line;
    std::size_t   column;
    std::string   message;
};

struct ValidationResult {
    ValidationStatus             status;
    std::vector<ValidationError> errors;
    std::string                  message;
};

#endif
