#ifndef SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_RUNTIME_H
#define SIMPLE_XML_VALIDATOR_INFRASTRUCTURE_XERCES_XERCES_RUNTIME_H

#include <string>

namespace simple_xml_validator::infrastructure::xerces {

class XercesRuntime final {
public:
    XercesRuntime();
    ~XercesRuntime();

    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] const std::string& errorMessage() const noexcept;

    XercesRuntime(const XercesRuntime&) = delete;
    XercesRuntime& operator=(const XercesRuntime&) = delete;

private:
    bool ready_;
    std::string error_;
};

}

#endif
