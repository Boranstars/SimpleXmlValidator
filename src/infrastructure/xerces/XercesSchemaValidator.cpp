#include "XercesSchemaValidator.h"

#include "LocalResourceResolver.h"
#include "XercesErrorHandler.h"
#include "XercesString.h"
#include "XercesUri.h"

#include <xercesc/dom/DOMException.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/SecurityManager.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/validators/common/Grammar.hpp>

#include <exception>

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
                     xercesc::XMLEntityResolver& resolver,
                     xercesc::SecurityManager& securityManager) {
    parser.setValidationScheme(xercesc::XercesDOMParser::Val_Always);
    parser.setDoNamespaces(true);
    parser.setDoSchema(true);
    parser.setValidationSchemaFullChecking(true);
    parser.setErrorHandler(&handler);
    parser.setXMLEntityResolver(&resolver);
    parser.setSecurityManager(&securityManager);
}

}

SchemaValidationReport XercesSchemaValidator::validate(
    const std::filesystem::path& xmlPath,
    const std::filesystem::path& xsdPath) {
    XercesErrorHandler    handler;
    LocalResourceResolver resolver;
    xercesc::SecurityManager securityManager;
    securityManager.setEntityExpansionLimit(100'000);

    xercesc::XercesDOMParser parser;
    configureParser(parser, handler, resolver, securityManager);

    const std::string xsdPathUtf8 = xsdPath.u8string();
    const std::string xmlPathUtf8 = xmlPath.u8string();

    // 已加载 XSD 的 targetNamespace；空字符串表示无命名空间 Schema。
    std::string targetNamespace;

    // 阶段一：预加载并缓存 XSD Grammar。
    try {
        ScopedXMLCh                   xsdLocation(xsdPathUtf8);
        xercesc::LocalFileInputSource xsdSource(xsdLocation.get());
        xercesc::Grammar* grammar = parser.loadGrammar(
            xsdSource, xercesc::Grammar::SchemaGrammarType, true);

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
            ScopedXMLCh xsdLocation(xsdPathUtf8);
            parser.setExternalNoNamespaceSchemaLocation(xsdLocation.get());
        } else {
            // setExternalSchemaLocation 要求 URI 而非原始路径；
            // 原始非 ASCII 路径在 Windows/macOS 上导致 schema 绑定失败。
            ScopedXMLCh schemaLocation(targetNamespace + " " + toFileUri(xsdPath));
            parser.setExternalSchemaLocation(schemaLocation.get());
        }
        parser.useCachedGrammarInParse(true);

        ScopedXMLCh                   xmlLocation(xmlPathUtf8);
        xercesc::LocalFileInputSource xmlSource(xmlLocation.get());
        parser.parse(xmlSource);

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
