#include "XercesTestFixture.h"

#include "infrastructure/xerces/XercesString.h"

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

}
