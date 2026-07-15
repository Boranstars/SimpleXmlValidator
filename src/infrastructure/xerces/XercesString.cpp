#include "XercesString.h"

#include <cstring>
#include <xercesc/util/XMLString.hpp>

namespace simple_xml_validator::infrastructure::xerces {

// TODO: XMLString::transcode uses the system locale code page, not explicitly
// UTF-8. On Linux (UTF-8 locale) this is fine. On Windows with a non-UTF-8
// code page (e.g. GBK), error messages containing non-ASCII XML content
// (such as Chinese element names) may be garbled. When Windows support is
// validated, replace with an explicit UTF-8 transcoder.
std::string fromXMLCh(const XMLCh* str) {
    if (!str) return {};
    char* transcoded = xercesc::XMLString::transcode(str);
    std::string result(transcoded ? transcoded : "");
    xercesc::XMLString::release(&transcoded);
    return result;
}

ScopedXMLCh::ScopedXMLCh(const char* str)
    : data_(str ? xercesc::XMLString::transcode(str) : nullptr) {}

ScopedXMLCh::ScopedXMLCh(const std::string& str)
    : data_(xercesc::XMLString::transcode(str.c_str())) {}

ScopedXMLCh::~ScopedXMLCh() {
    if (data_) xercesc::XMLString::release(&data_);
}

const XMLCh* ScopedXMLCh::get() const noexcept {
    return data_;
}

}
