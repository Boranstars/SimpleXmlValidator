#include "XercesTestFixture.h"

#include "infrastructure/xerces/XercesString.h"

#include <filesystem>
#include <string>

namespace {

using simple_xml_validator::infrastructure::xerces::ScopedXMLCh;
using simple_xml_validator::infrastructure::xerces::fromXMLCh;

void expectRoundTrip(const std::string& input) {
    ScopedXMLCh encoded(input);
    ASSERT_NE(encoded.get(), nullptr);
    EXPECT_EQ(fromXMLCh(encoded.get()), input);
}

TEST_F(XercesTestFixture, ConvertsNullXmlChToEmptyString) {
    EXPECT_TRUE(fromXMLCh(nullptr).empty());
}

TEST_F(XercesTestFixture, PreservesAsciiText) {
    expectRoundTrip("schema error 42");
}

TEST_F(XercesTestFixture, PreservesEmptyStringAsNonNullXmlCh) {
    const std::string input;
    ScopedXMLCh encoded(input);

    ASSERT_NE(encoded.get(), nullptr);
    EXPECT_TRUE(fromXMLCh(encoded.get()).empty());
}

TEST_F(XercesTestFixture, PreservesChineseUtf8Text) {
    expectRoundTrip(u8"中文元素错误");
}

TEST_F(XercesTestFixture, PreservesMixedUtf8Text) {
    expectRoundTrip(u8"节点 node-42，值=alpha_1!");
}

TEST_F(XercesTestFixture, DistinguishesNullAndEmptyCStringInputs) {
    ScopedXMLCh nullInput(nullptr);
    ScopedXMLCh emptyInput("");

    EXPECT_EQ(nullInput.get(), nullptr);
    ASSERT_NE(emptyInput.get(), nullptr);
    EXPECT_TRUE(fromXMLCh(emptyInput.get()).empty());
}

TEST_F(XercesTestFixture, RepeatedScopedConversionsRemainStable) {
    for (std::size_t iteration = 0; iteration < 256; ++iteration) {
        ScopedXMLCh encoded(u8"重复转换测试");
        ASSERT_NE(encoded.get(), nullptr);
        EXPECT_EQ(fromXMLCh(encoded.get()), u8"重复转换测试");
    }
}

#if defined(_WIN32)
TEST_F(XercesTestFixture, UsesUtf8ForWindowsDiagnosticTextRegardlessOfActiveCodePage) {
    const XMLCh diagnostic[] = {
        static_cast<XMLCh>(0x4E2D), static_cast<XMLCh>(0x6587),
        static_cast<XMLCh>(0x5143), static_cast<XMLCh>(0x7D20),
        static_cast<XMLCh>(0x9519), static_cast<XMLCh>(0x8BEF),
        static_cast<XMLCh>(0),
    };

    EXPECT_EQ(fromXMLCh(diagnostic), u8"中文元素错误");
}

TEST_F(XercesTestFixture, ConvertsWindowsUtf8InputToExpectedUtf16CodeUnits) {
    ScopedXMLCh encoded(u8"中文");

    ASSERT_NE(encoded.get(), nullptr);
    EXPECT_EQ(encoded.get()[0], static_cast<XMLCh>(0x4E2D));
    EXPECT_EQ(encoded.get()[1], static_cast<XMLCh>(0x6587));
    EXPECT_EQ(encoded.get()[2], static_cast<XMLCh>(0));
}

TEST_F(XercesTestFixture, PreservesWindowsUtf8NonBmpCharacterAsSurrogatePair) {
    const XMLCh character[] = {
        static_cast<XMLCh>(0xD83D), static_cast<XMLCh>(0xDE00), static_cast<XMLCh>(0),
    };
    ScopedXMLCh encoded(u8"😀");

    EXPECT_EQ(fromXMLCh(character), u8"😀");
    ASSERT_NE(encoded.get(), nullptr);
    EXPECT_EQ(encoded.get()[0], static_cast<XMLCh>(0xD83D));
    EXPECT_EQ(encoded.get()[1], static_cast<XMLCh>(0xDE00));
    EXPECT_EQ(encoded.get()[2], static_cast<XMLCh>(0));
}

TEST_F(XercesTestFixture, PreservesWindowsUtf8FilesystemPathRoundTrip) {
    const std::string utf8Path = u8"目录 é.xsd";
    const std::filesystem::path path = std::filesystem::u8path(utf8Path);

    EXPECT_EQ(path.u8string(), utf8Path);
}

TEST_F(XercesTestFixture, ConvertsWindowsUtf8FilesystemPathToExpectedUtf16CodeUnits) {
    const std::filesystem::path path = std::filesystem::u8path(u8"目录 é.xsd");
    ScopedXMLCh encoded(path);

    ASSERT_NE(encoded.get(), nullptr);
    EXPECT_EQ(encoded.get()[0], static_cast<XMLCh>(0x76EE));
    EXPECT_EQ(encoded.get()[1], static_cast<XMLCh>(0x5F55));
    EXPECT_EQ(encoded.get()[2], static_cast<XMLCh>(0x0020));
    EXPECT_EQ(encoded.get()[3], static_cast<XMLCh>(0x00E9));
    EXPECT_EQ(encoded.get()[4], static_cast<XMLCh>(0x002E));
    EXPECT_EQ(encoded.get()[5], static_cast<XMLCh>(0x0078));
    EXPECT_EQ(encoded.get()[6], static_cast<XMLCh>(0x0073));
    EXPECT_EQ(encoded.get()[7], static_cast<XMLCh>(0x0064));
    EXPECT_EQ(encoded.get()[8], static_cast<XMLCh>(0));
}
#endif

}
