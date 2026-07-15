#include "XercesErrorHandler.h"

#include "XercesString.h"

#include <utility>

namespace simple_xml_validator::infrastructure::xerces {

void XercesErrorHandler::warning(const xercesc::SAXParseException& exception) {
    append(ErrorSeverity::Warning, exception);
}

void XercesErrorHandler::error(const xercesc::SAXParseException& exception) {
    append(ErrorSeverity::Error, exception);
}

void XercesErrorHandler::fatalError(const xercesc::SAXParseException& exception) {
    append(ErrorSeverity::Fatal, exception);
}

void XercesErrorHandler::resetErrors() {
    errors_.clear();
}

std::vector<ValidationError> XercesErrorHandler::takeErrors() {
    return std::move(errors_);
}

void XercesErrorHandler::append(ErrorSeverity severity, const xercesc::SAXParseException& exception) {
    errors_.push_back({
        severity,
        static_cast<std::size_t>(exception.getLineNumber()),
        static_cast<std::size_t>(exception.getColumnNumber()),
        fromXMLCh(exception.getMessage())
    });
}

}
