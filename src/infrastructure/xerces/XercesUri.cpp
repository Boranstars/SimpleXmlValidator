#include "XercesUri.h"

#include <cctype>
#include <string_view>

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

std::string percentDecodeUri(std::string_view encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const auto hexDigit = [](char c) -> unsigned char {
                if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
                if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
                if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
                return 0;
            };
            const unsigned char byte =
                static_cast<unsigned char>((hexDigit(encoded[i + 1]) << 4) |
                                            hexDigit(encoded[i + 2]));
            decoded.push_back(static_cast<char>(byte));
            i += 2;
        } else {
            decoded.push_back(encoded[i]);
        }
    }
    return decoded;
}

}

std::string toFileUri(const std::filesystem::path& path) {
    const auto        normalized  = std::filesystem::absolute(path).lexically_normal();
    const std::string genericPath = normalized.generic_u8string();
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

std::filesystem::path pathFromFileUri(std::string_view fileUri) {
    static constexpr std::string_view kPrefix = "file://";
    if (fileUri.substr(0, kPrefix.size()) != kPrefix) {
        return {};
    }
    std::string_view pathPart = fileUri.substr(kPrefix.size());

#if defined(_WIN32)
    // file:///C:/path → pathPart 为 "/C:/path"，需去掉首 "/"
    if (pathPart.size() >= 3 && pathPart[0] == '/' && pathPart[2] == ':') {
        pathPart.remove_prefix(1);
    }
#endif

    // 必须用 u8path 而非 path(string)：后者在 Windows 上用 ANSI codepage，
    // 不能正确解析 percent-decode 后含中文或 é 的 UTF-8 字节序列。
    return std::filesystem::u8path(percentDecodeUri(pathPart));
}

}
