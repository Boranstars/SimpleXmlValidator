#include "XercesSchemaValidator.h"

#include "LocalResourceResolver.h"
#include "XercesErrorHandler.h"
#include "XercesString.h"

#include <xercesc/dom/DOMException.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
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
                     xercesc::XMLEntityResolver& resolver) {
    parser.setValidationScheme(xercesc::XercesDOMParser::Val_Always);
    parser.setDoNamespaces(true);
    parser.setDoSchema(true);
    parser.setValidationSchemaFullChecking(true);
    parser.setErrorHandler(&handler);
    parser.setXMLEntityResolver(&resolver);
}

}

SchemaValidationReport XercesSchemaValidator::validate(
    const std::filesystem::path& xmlPath,
    const std::filesystem::path& xsdPath) {
    XercesErrorHandler    handler;
    LocalResourceResolver resolver;

    xercesc::XercesDOMParser parser;
    configureParser(parser, handler, resolver);

    const std::string xsdPathUtf8 = xsdPath.u8string();

    // 已加载 XSD 的 targetNamespace；空字符串表示无命名空间 Schema。
    std::string targetNamespace;

    // 主文件用 ScopedXMLCh(path) 构造：在 Windows 上直接使用 path.native()（UTF-16），
    // 避免 MSVC u8string() 的 ANSI codepage bug；在其他平台使用 u8string()。
    try {
        ScopedXMLCh                   xsdLocation(xsdPath);
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
            ScopedXMLCh xsdLocation(xsdPath);
            parser.setExternalNoNamespaceSchemaLocation(xsdLocation.get());
        } else {
            ScopedXMLCh schemaLocation(targetNamespace + " " + xsdPathUtf8);
            parser.setExternalSchemaLocation(schemaLocation.get());
        }
        parser.useCachedGrammarInParse(true);

        ScopedXMLCh                   xmlLocation(xmlPath);
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
