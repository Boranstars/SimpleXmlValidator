#include "XercesUri.h"

#include <cctype>
#include <string_view>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace simple_xml_validator::infrastructure::xerces {

namespace {

bool isUnreservedUriChar(unsigned char c) {
    return std::isalnum(c) != 0 || c == '-' || c == '.' || c == '_' || c == '~';
}

std::string percentEncodeUriPath(std::string_view pathUtf8) {
    static constexpr char kHexDigits[] = "0123456789ABCDEF";
    std::string           encoded;
    encoded.reserve(pathUtf8.size() * 3);
    for (const unsigned char c : pathUtf8) {
        if (isUnreservedUriChar(c) || c == '/' || c == ':') {
            encoded.push_back(static_cast<char>(c));
            continue;
        }
        encoded.push_back('%');
        encoded.push_back(kHexDigits[(c >> 4) & 0x0F]);
        encoded.push_back(kHexDigits[c & 0x0F]);
    }
    return encoded;
}

}

std::string toFileUri(const std::filesystem::path& path) {
    const auto normalized = std::filesystem::absolute(path).lexically_normal();

#if defined(_WIN32)
    // Windows MSVC: generic_u8string() 有 bug，会把 UTF-8 字节误当成本地代码页字符。
    // 改用 generic_wstring() + WideCharToMultiByte 安全转为 UTF-8。
    const std::wstring wpath = normalized.generic_wstring();
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.size()),
        nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string genericPath(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.size()),
                        genericPath.data(), needed, nullptr, nullptr);
#else
    const std::string genericPath = normalized.generic_u8string();
#endif

    const std::string encodedPath = percentEncodeUriPath(genericPath);

#if defined(_WIN32)
    if (genericPath.rfind("//", 0) == 0) {
        return "file:" + encodedPath;
    }
    return "file:///" + encodedPath;
#else
    return "file://" + encodedPath;
#endif
}

}
