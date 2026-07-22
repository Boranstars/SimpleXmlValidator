#include "XercesString.h"

#include <cstring>
#include <xercesc/util/XMLString.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <xercesc/util/PlatformUtils.hpp>
#endif

namespace simple_xml_validator::infrastructure::xerces {

std::string fromXMLCh(const XMLCh* str) {
    if (!str) return {};
#if defined(_WIN32)
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, reinterpret_cast<LPCWSTR>(str), -1,
        nullptr, 0, nullptr, nullptr);
    if (needed <= 1) return {};
    std::string result(needed - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWSTR>(str), -1,
        result.data(), needed - 1, nullptr, nullptr);
    return result;
#else
    char* transcoded = xercesc::XMLString::transcode(str);
    std::string result(transcoded ? transcoded : "");
    xercesc::XMLString::release(&transcoded);
    return result;
#endif
}

ScopedXMLCh::ScopedXMLCh(const char* str) : data_(nullptr) {
    if (!str) return;
#if defined(_WIN32)
    const int srcLen = static_cast<int>(strlen(str));
    if (srcLen == 0) {
        data_ = static_cast<XMLCh*>(
            xercesc::XMLPlatformUtils::fgMemoryManager->allocate(sizeof(XMLCh)));
        data_[0] = 0;
        return;
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, str, srcLen, nullptr, 0);
    if (needed <= 0) return;
    data_ = static_cast<XMLCh*>(
        xercesc::XMLPlatformUtils::fgMemoryManager->allocate(
            (needed + 1) * sizeof(XMLCh)));
    MultiByteToWideChar(CP_UTF8, 0, str, srcLen,
        reinterpret_cast<LPWSTR>(data_), needed);
    data_[needed] = 0;
#else
    data_ = xercesc::XMLString::transcode(str);
#endif
}

ScopedXMLCh::ScopedXMLCh(const std::string& str)
    : ScopedXMLCh(str.c_str()) {}

ScopedXMLCh::~ScopedXMLCh() {
    if (data_) xercesc::XMLString::release(&data_);
}

const XMLCh* ScopedXMLCh::get() const noexcept {
    return data_;
}

}
