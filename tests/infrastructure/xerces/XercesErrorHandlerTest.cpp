#include "XercesTestFixture.h"

#include "infrastructure/xerces/XercesErrorHandler.h"
#include "infrastructure/xerces/XercesString.h"

#include <xercesc/sax/SAXParseException.hpp>

#include <string>

namespace {

using simple_xml_validator::infrastructure::xerces::ScopedXMLCh;
using simple_xml_validator::infrastructure::xerces::XercesErrorHandler;

xercesc::SAXParseException makeParseException(
    const std::string& message,
    XMLFileLoc line,
    XMLFileLoc column) {
    ScopedXMLCh encodedMessage(message);
    ScopedXMLCh publicId("test-public-id");
    ScopedXMLCh systemId("test-system-id.xml");

    return {
        encodedMessage.get(),
        publicId.get(),
        systemId.get(),
        line,
        column
    };
}

TEST_F(XercesTestFixture, MapsWarningErrorAndFatalSeveritiesInOrder) {
    XercesErrorHandler handler;
    const auto warning = makeParseException("warning message", 3, 5);
    const auto error = makeParseException("error message", 7, 11);
    const auto fatal = makeParseException("fatal message", 13, 17);

    handler.warning(warning);
    handler.error(error);
    handler.fatalError(fatal);

    const auto errors = handler.takeErrors();

    ASSERT_EQ(errors.size(), 3U);
    EXPECT_EQ(errors[0].severity, ErrorSeverity::Warning);
    EXPECT_EQ(errors[0].line, 3U);
    EXPECT_EQ(errors[0].column, 5U);
    EXPECT_EQ(errors[0].message, "warning message");
    EXPECT_EQ(errors[1].severity, ErrorSeverity::Error);
    EXPECT_EQ(errors[1].line, 7U);
    EXPECT_EQ(errors[1].column, 11U);
    EXPECT_EQ(errors[1].message, "error message");
    EXPECT_EQ(errors[2].severity, ErrorSeverity::Fatal);
    EXPECT_EQ(errors[2].line, 13U);
    EXPECT_EQ(errors[2].column, 17U);
    EXPECT_EQ(errors[2].message, "fatal message");
}

TEST_F(XercesTestFixture, PreservesChineseMessage) {
    XercesErrorHandler handler;
    const std::string message = u8"元素名称不符合约束";
    const auto exception = makeParseException(message, 19, 23);

    handler.error(exception);

    const auto errors = handler.takeErrors();

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front().message, message);
}

TEST_F(XercesTestFixture, ReturnsEmptyListAfterErrorsAreTaken) {
    XercesErrorHandler handler;
    const auto exception = makeParseException("first error", 1, 2);

    handler.error(exception);

    const auto firstErrors = handler.takeErrors();
    const auto secondErrors = handler.takeErrors();

    ASSERT_EQ(firstErrors.size(), 1U);
    EXPECT_TRUE(secondErrors.empty());
}

TEST_F(XercesTestFixture, ClearsUnreleasedErrorsWhenReset) {
    XercesErrorHandler handler;
    const auto exception = makeParseException("reset error", 29, 31);

    handler.error(exception);
    handler.resetErrors();

    EXPECT_TRUE(handler.takeErrors().empty());
}

TEST_F(XercesTestFixture, RetainsOnlyErrorsCollectedAfterReset) {
    XercesErrorHandler handler;
    const auto discarded = makeParseException("discarded error", 37, 41);
    const auto retained = makeParseException("retained error", 43, 47);

    handler.warning(discarded);
    handler.resetErrors();
    handler.fatalError(retained);

    const auto errors = handler.takeErrors();

    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front().severity, ErrorSeverity::Fatal);
    EXPECT_EQ(errors.front().line, 43U);
    EXPECT_EQ(errors.front().column, 47U);
    EXPECT_EQ(errors.front().message, "retained error");
}

}
