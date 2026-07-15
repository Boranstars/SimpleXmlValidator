#include "XercesRuntime.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/XMLString.hpp>

namespace simple_xml_validator::infrastructure::xerces {

XercesRuntime::XercesRuntime() : ready_(false) {
    try {
        xercesc::XMLPlatformUtils::Initialize();
        ready_ = true;
    } catch (const xercesc::XMLException& e) {
        // Transcoding services may be partially initialized; attempt conversion
        // but fall back to a generic message if it fails.
        try {
            char* msg = xercesc::XMLString::transcode(e.getMessage());
            if (msg) {
                error_ = std::string("Xerces 初始化失败：") + msg;
                xercesc::XMLString::release(&msg);
            } else {
                error_ = "Xerces 初始化失败";
            }
        } catch (...) {
            error_ = "Xerces 初始化失败";
        }
    } catch (const std::exception& e) {
        error_ = std::string("Xerces 初始化失败：") + e.what();
    } catch (...) {
        error_ = "Xerces 初始化失败（未知异常）";
    }
}

XercesRuntime::~XercesRuntime() {
    if (ready_) {
        xercesc::XMLPlatformUtils::Terminate();
    }
}

bool XercesRuntime::isReady() const noexcept {
    return ready_;
}

const std::string& XercesRuntime::errorMessage() const noexcept {
    return error_;
}

}
