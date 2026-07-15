#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_STRING_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_STRING_H

#include <string>
#include <xercesc/util/XercesDefs.hpp>

namespace simple_xml_validator::infrastructure::xerces {

std::string fromXMLCh(const XMLCh* str);

class ScopedXMLCh {
public:
    explicit ScopedXMLCh(const char* str);
    explicit ScopedXMLCh(const std::string& str);
    ~ScopedXMLCh();

    [[nodiscard]] const XMLCh* get() const noexcept;

    ScopedXMLCh(const ScopedXMLCh&) = delete;
    ScopedXMLCh& operator=(const ScopedXMLCh&) = delete;

private:
    XMLCh* data_;
};

}

#endif
