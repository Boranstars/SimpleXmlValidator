#include "LocalResourceResolver.h"

#include "XercesString.h"

#include <cctype>
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

    // 本地相对/绝对路径交由 Xerces 基于 baseURI 默认解析；
    // 依赖缺失或不可读由解析器上报给错误处理器。
    return nullptr;
}

bool LocalResourceResolver::hasError() const noexcept {
    return hadError_;
}

const std::string& LocalResourceResolver::errorMessage() const noexcept {
    return error_;
}

}
