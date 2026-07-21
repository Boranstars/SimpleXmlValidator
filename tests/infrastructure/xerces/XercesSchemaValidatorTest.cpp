#include "XercesTestFixture.h"

#include "infrastructure/xerces/XercesSchemaValidator.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using simple_xml_validator::infrastructure::xerces::SchemaValidationStage;
using simple_xml_validator::infrastructure::xerces::XercesSchemaValidator;

class XercesSchemaValidatorTest : public XercesTestFixture {
protected:
    void SetUp() override {
        XercesTestFixture::SetUp();
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temporaryDirectory_ = std::filesystem::temp_directory_path()
                              / ("simple_xml_validator_schema_validator_" + std::to_string(timestamp));
        std::filesystem::create_directories(temporaryDirectory_);
    }

    void TearDown() override {
        std::error_code errorCode;
        std::filesystem::remove_all(temporaryDirectory_, errorCode);
    }

    void writeFile(const std::filesystem::path& path, const std::string& content) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        ASSERT_TRUE(output.is_open());
        output << content;
        ASSERT_TRUE(output.good());
    }

    std::filesystem::path temporaryDirectory_;
};

TEST_F(XercesSchemaValidatorTest, CompletesForSchemaWithRelativeInclude) {
    const auto xsdPath = std::filesystem::path(SIMPLE_XML_VALIDATOR_SOURCE_ROOT)
                         / "tests/xsd/setting_check.xsd";
    const auto xmlPath = std::filesystem::path(SIMPLE_XML_VALIDATOR_SOURCE_ROOT)
                         / "tests/xml/valid/A2_valid_simple_ok.xml";

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Completed);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(XercesSchemaValidatorTest, BlocksSchemaWithMissingLocalInclude) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:include schemaLocation=\"missing.xsd\"/></xs:schema>");
    writeFile(xmlPath, "<root/>");

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Blocked);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_FALSE(result.message.empty());
}

TEST_F(XercesSchemaValidatorTest, BlocksMalformedSchema) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:element name=\"root\"></xs:schema>");
    writeFile(xmlPath, "<root/>");

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Blocked);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_NE(result.message.find("XSD Schema 加载失败"), std::string::npos);
}

TEST_F(XercesSchemaValidatorTest, BlocksRemoteSchemaDependency) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:include schemaLocation=\"https://example.invalid/dependency.xsd\"/>"
              "</xs:schema>");
    writeFile(xmlPath, "<root/>");

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Blocked);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_NE(result.message.find("拒绝访问网络资源"), std::string::npos);
}

#if defined(__APPLE__)
TEST_F(XercesSchemaValidatorTest, CompletesForUtf8LocalSchemaDependencyPathOnMacOS) {
    const auto schemaDirectory = temporaryDirectory_ / u8"macOS 架构 é";
    const auto dependencyPath = schemaDirectory / u8"公共依赖 é.xsd";
    const auto xsdPath = schemaDirectory / u8"主架构 é.xsd";
    const auto xmlPath = schemaDirectory / u8"示例文档 é.xml";
    writeFile(dependencyPath,
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
              "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:element name=\"根\" type=\"xs:string\"/>"
              "</xs:schema>");
    writeFile(xsdPath,
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
              "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:include schemaLocation=\"公共依赖 é.xsd\"/>"
              "</xs:schema>");
    writeFile(xmlPath, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><根>有效</根>");

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Completed) << "message: " << result.message;
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(result.message.empty()) << "message: " << result.message;
}
#endif

#if defined(_WIN32)
TEST_F(XercesSchemaValidatorTest, CompletesForUtf8LocalSchemaDependencyPathOnWindows) {
    const auto schemaDirectory = temporaryDirectory_ / u8"Windows 架构 é";
    const auto dependencyPath = schemaDirectory / u8"公共依赖 é.xsd";
    const auto xsdPath = schemaDirectory / u8"主架构 é.xsd";
    const auto xmlPath = schemaDirectory / u8"示例文档 é.xml";
    writeFile(dependencyPath,
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
              "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:element name=\"根\" type=\"xs:string\"/>"
              "</xs:schema>");
    writeFile(xsdPath,
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
              "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:include schemaLocation=\"公共依赖 é.xsd\"/>"
              "</xs:schema>");
    writeFile(xmlPath, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><根>有效</根>");

    XercesSchemaValidator validator;
    const auto result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.stage, SchemaValidationStage::Completed) << "message: " << result.message;
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(result.message.empty()) << "message: " << result.message;
}
#endif

}
