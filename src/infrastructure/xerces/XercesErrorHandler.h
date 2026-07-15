#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_ERROR_HANDLER_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_ERROR_HANDLER_H

#include "core/validation/ValidationResult.h"

#include <vector>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

namespace simple_xml_validator::infrastructure::xerces {

class XercesErrorHandler final : public xercesc::ErrorHandler {
public:
    void warning(const xercesc::SAXParseException& exception) override;
    void error(const xercesc::SAXParseException& exception) override;
    void fatalError(const xercesc::SAXParseException& exception) override;
    void resetErrors() override;

    std::vector<ValidationError> takeErrors();

private:
    void append(ErrorSeverity severity, const xercesc::SAXParseException& exception);
    std::vector<ValidationError> errors_;
};

}

#endif
