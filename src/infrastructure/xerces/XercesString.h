#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_STRING_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_STRING_H

#include <filesystem>
#include <string>
#include <xercesc/util/XercesDefs.hpp>

namespace simple_xml_validator::infrastructure::xerces {

std::string fromXMLCh(const XMLCh* str);

class ScopedXMLCh {
public:
    explicit ScopedXMLCh(const char* str);
    explicit ScopedXMLCh(const std::string& str);
    // Windows 上直接用 path.native()（wchar_t UTF-16），跳过有 bug 的 u8string()。
    explicit ScopedXMLCh(const std::filesystem::path& path);
    ~ScopedXMLCh();

    [[nodiscard]] const XMLCh* get() const noexcept;

    ScopedXMLCh(const ScopedXMLCh&) = delete;
    ScopedXMLCh& operator=(const ScopedXMLCh&) = delete;

private:
    XMLCh* data_;
};

}

#endif
