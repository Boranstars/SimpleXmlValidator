#include "LocalResourceResolver.h"

#include "XercesString.h"
#include "XercesUri.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/XMLResourceIdentifier.hpp>

namespace simple_xml_validator::infrastructure::xerces {

namespace {

// 判断资源标识是否带有远程 URI scheme（如 http:、https:、ftp:）。
// 无 scheme 的相对路径和 file: 视为本地资源。
bool isRemoteScheme(const std::string& systemId) {
    const auto pos = systemId.find("://");
    if (pos == std::string::npos || pos == 0) {
        return false;
    }
    std::string scheme = systemId.substr(0, pos);
    for (char& c : scheme) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return scheme != "file";
}

}

xercesc::InputSource* LocalResourceResolver::resolveEntity(
    xercesc::XMLResourceIdentifier* resourceIdentifier) {
    if (!resourceIdentifier) {
        return nullptr;
    }

    const std::string systemId = fromXMLCh(resourceIdentifier->getSystemId());
    if (isRemoteScheme(systemId)) {
        hadError_ = true;
        error_ = "拒绝访问网络资源：" + systemId;
        // 返回空内容输入源，阻止 Xerces 发起网络请求；Schema 预加载随后失败。
        static const XMLByte kEmpty[] = {0};
        return new xercesc::MemBufInputSource(
            kEmpty, 0, resourceIdentifier->getSystemId(), false);
    }

    // 仅拦截含非 ASCII 字节的相对 schemaLocation：Xerces 的 URI 解析器无法把
    // 未 percent-encode 的原始 Unicode 字符解析为合法的相对 URI 引用。
    // ASCII 路径继续返回 nullptr，由 Xerces 基于 file:// baseURI 自行解析（正常工作）。
    const bool systemIdHasScheme    = systemId.find("://") != std::string::npos;
    const bool systemIdHasNonAscii  = std::any_of(systemId.begin(), systemId.end(),
        [](unsigned char c) { return c > 0x7F; });

    if (!systemId.empty() && !systemIdHasScheme && systemIdHasNonAscii) {
        const std::string     baseUri = fromXMLCh(resourceIdentifier->getBaseURI());
        std::filesystem::path parentDir;

        if (baseUri.rfind("file://", 0) == 0) {
            parentDir = pathFromFileUri(baseUri).parent_path();
        } else if (!baseUri.empty()) {
            // getBaseURI() 在某些平台/Xerces 版本可能直接返回本地路径而非 file:// URI
            try { parentDir = std::filesystem::path(baseUri).parent_path(); } catch (...) {}
        }

        if (!parentDir.empty()) {
            // u8path 确保 UTF-8 字符串在 Windows 上也被正确解析为 Unicode 路径
            const auto absolutePath =
                (parentDir / std::filesystem::u8path(systemId)).lexically_normal();
            std::ifstream file(absolutePath, std::ios::binary);
            if (file) {
                std::string content(std::istreambuf_iterator<char>(file),
                                    std::istreambuf_iterator<char>{});
                const std::string fileUri = toFileUri(absolutePath);
                ScopedXMLCh      xmlChUri(fileUri);
                auto*            buf = new XMLByte[content.size()];
                std::copy(content.begin(), content.end(), reinterpret_cast<char*>(buf));
                // adoptBuffer=true：Xerces 负责 delete[] buf
                return new xercesc::MemBufInputSource(
                    buf, content.size(), xmlChUri.get(), true);
            }
        }
    }

    return nullptr;
}

bool LocalResourceResolver::hasError() const noexcept {
    return hadError_;
}

const std::string& LocalResourceResolver::errorMessage() const noexcept {
    return error_;
}

}
