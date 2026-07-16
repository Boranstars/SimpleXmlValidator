#ifndef SIMPLE_XML_VALIDATOR_CORE_VALIDATION_XML_VALIDATOR_H
#define SIMPLE_XML_VALIDATOR_CORE_VALIDATION_XML_VALIDATOR_H

#include "core/validation/ValidationResult.h"

#include <filesystem>

namespace simple_xml_validator::infrastructure::xerces {
class XercesRuntime;
}

namespace simple_xml_validator::infrastructure::logging {
class LogManager;
class LogModule;
}

class XmlValidator {
public:
    // logManager 为可选的共享日志器；为 nullptr 时校验流程正常执行且不写日志。
    explicit XmlValidator(
        const simple_xml_validator::infrastructure::xerces::XercesRuntime& runtime,
        simple_xml_validator::infrastructure::logging::LogManager*         logManager = nullptr);

    ValidationResult validate(
        const std::filesystem::path& xmlPath,
        const std::filesystem::path& xsdPath);

private:
    ValidationResult runValidation(
        const std::filesystem::path&                                    xmlPath,
        const std::filesystem::path&                                    xsdPath,
        const simple_xml_validator::infrastructure::logging::LogModule& systemLog);

    const simple_xml_validator::infrastructure::xerces::XercesRuntime* runtime_;
    simple_xml_validator::infrastructure::logging::LogManager*         logManager_;
};

#endif
