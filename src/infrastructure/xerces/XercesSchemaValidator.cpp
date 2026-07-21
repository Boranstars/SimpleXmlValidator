#include "XercesSchemaValidator.h"

#include "LocalResourceResolver.h"
#include "XercesErrorHandler.h"
#include "XercesString.h"

#include <xercesc/dom/DOMException.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/validators/common/Grammar.hpp>

#include <cctype>
#include <exception>
#include <string_view>

namespace simple_xml_validator::infrastructure::xerces {

namespace {

// 将一批诊断汇总为一句可读消息，用于 Blocked 阶段的 message。
std::string summarize(const std::vector<ValidationError>& errors) {
    for (const auto& e : errors) {
        if (!e.message.empty()) {
            return e.message;
        }
    }
    return {};
}

void configureParser(xercesc::XercesDOMParser& parser,
                     xercesc::ErrorHandler& handler,
                     xercesc::XMLEntityResolver& resolver) {
    parser.setValidationScheme(xercesc::XercesDOMParser::Val_Always);
    parser.setDoNamespaces(true);
    parser.setDoSchema(true);
    parser.setValidationSchemaFullChecking(true);
    parser.setErrorHandler(&handler);
    parser.setXMLEntityResolver(&resolver);
}

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

std::string toFileUri(const std::filesystem::path& path) {
    const auto        normalized = std::filesystem::absolute(path).lexically_normal();
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

}

SchemaValidationReport XercesSchemaValidator::validate(
    const std::filesystem::path& xmlPath,
    const std::filesystem::path& xsdPath) {
    XercesErrorHandler    handler;
    LocalResourceResolver resolver;

    xercesc::XercesDOMParser parser;
    configureParser(parser, handler, resolver);

    const std::string xsdPathUri = toFileUri(xsdPath);
    const std::string xmlPathUri = toFileUri(xmlPath);

    // 已加载 XSD 的 targetNamespace；空字符串表示无命名空间 Schema。
    std::string targetNamespace;

    // 阶段一：预加载并缓存 XSD Grammar。
    try {
        ScopedXMLCh         xsdLocation(xsdPathUri);
        xercesc::Grammar* grammar = parser.loadGrammar(
            xsdLocation.get(), xercesc::Grammar::SchemaGrammarType, true);

        auto schemaErrors = handler.takeErrors();
        if (resolver.hasError()) {
            return {SchemaValidationStage::Blocked, {}, resolver.errorMessage()};
        }
        if (grammar == nullptr || !schemaErrors.empty()) {
            std::string detail = summarize(schemaErrors);
            std::string message = "XSD Schema 加载失败";
            if (!detail.empty()) {
                message += "：" + detail;
            }
            return {SchemaValidationStage::Blocked, {}, message};
        }
        targetNamespace = fromXMLCh(grammar->getTargetNamespace());
    } catch (const xercesc::OutOfMemoryException&) {
        return {SchemaValidationStage::Blocked, {}, "XSD Schema 加载失败：内存不足"};
    } catch (const xercesc::XMLException& e) {
        return {SchemaValidationStage::Blocked, {},
                "XSD Schema 加载失败：" + fromXMLCh(e.getMessage())};
    } catch (const xercesc::DOMException& e) {
        return {SchemaValidationStage::Blocked, {},
                "XSD Schema 加载失败：" + fromXMLCh(e.msg)};
    } catch (const std::exception& e) {
        return {SchemaValidationStage::Blocked, {},
                std::string("XSD Schema 加载失败：") + e.what()};
    } catch (...) {
        return {SchemaValidationStage::Blocked, {}, "XSD Schema 加载失败：未知异常"};
    }

    // 阶段二：基于已缓存 Grammar 解析并校验 XML。
    try {
        handler.resetErrors();
        // 依据 XSD 是否带 targetNamespace 选择合适的外部 Schema 绑定方式。
        if (targetNamespace.empty()) {
            ScopedXMLCh xsdLocation(xsdPathUri);
            parser.setExternalNoNamespaceSchemaLocation(xsdLocation.get());
        } else {
            ScopedXMLCh schemaLocation(targetNamespace + " " + xsdPathUri);
            parser.setExternalSchemaLocation(schemaLocation.get());
        }
        parser.useCachedGrammarInParse(true);

        ScopedXMLCh xmlLocation(xmlPathUri);
        parser.parse(xmlLocation.get());

        return {SchemaValidationStage::Completed, handler.takeErrors(), {}};
    } catch (const xercesc::OutOfMemoryException&) {
        return {SchemaValidationStage::Blocked, {}, "XML 校验失败：内存不足"};
    } catch (const xercesc::XMLException& e) {
        return {SchemaValidationStage::Blocked, {},
                "XML 校验引擎异常：" + fromXMLCh(e.getMessage())};
    } catch (const xercesc::DOMException& e) {
        return {SchemaValidationStage::Blocked, {},
                "XML 校验引擎异常：" + fromXMLCh(e.msg)};
    } catch (const std::exception& e) {
        return {SchemaValidationStage::Blocked, {},
                std::string("XML 校验引擎异常：") + e.what()};
    } catch (...) {
        return {SchemaValidationStage::Blocked, {}, "XML 校验引擎异常：未知异常"};
    }
}

}
