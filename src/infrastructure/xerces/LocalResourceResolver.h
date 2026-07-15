#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_LOCAL_RESOURCE_RESOLVER_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_LOCAL_RESOURCE_RESOLVER_H

#include <string>
#include <xercesc/util/XMLEntityResolver.hpp>

namespace simple_xml_validator::infrastructure::xerces {

// 为 XSD 的 include/import 提供本地资源解析：相对路径交由 Xerces 默认解析，
// 网络资源一律拒绝并记录原因，避免 Schema 预加载阶段访问网络。
class LocalResourceResolver final : public xercesc::XMLEntityResolver {
public:
    xercesc::InputSource* resolveEntity(
        xercesc::XMLResourceIdentifier* resourceIdentifier) override;

    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const std::string& errorMessage() const noexcept;

private:
    bool        hadError_ = false;
    std::string error_;
};

}

#endif
