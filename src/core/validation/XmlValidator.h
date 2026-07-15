#ifndef SIMPLE_XML_VALIDATOR_CORE_VALIDATION_XML_VALIDATOR_H
#define SIMPLE_XML_VALIDATOR_CORE_VALIDATION_XML_VALIDATOR_H

#include "core/validation/ValidationResult.h"

#include <filesystem>

namespace simple_xml_validator::infrastructure::xerces {
class XercesRuntime;
}

class XmlValidator {
public:
    explicit XmlValidator(
        const simple_xml_validator::infrastructure::xerces::XercesRuntime& runtime);

    ValidationResult validate(
        const std::filesystem::path& xmlPath,
        const std::filesystem::path& xsdPath);

private:
    const simple_xml_validator::infrastructure::xerces::XercesRuntime* runtime_;
};

#endif
