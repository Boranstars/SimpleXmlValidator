#include "../../infrastructure/xerces/XercesTestFixture.h"

#include "core/validation/XmlValidator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace {

class XmlValidatorTest : public XercesTestFixture {
protected:
    void SetUp() override {
        XercesTestFixture::SetUp();
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temporaryDirectory_ = std::filesystem::temp_directory_path()
                              / ("simple_xml_validator_xml_validator_" + std::to_string(timestamp));
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

    std::filesystem::path fixturePath(const std::filesystem::path& relativePath) const {
        return std::filesystem::path(SIMPLE_XML_VALIDATOR_SOURCE_ROOT) / relativePath;
    }

    std::filesystem::path temporaryDirectory_;
};

TEST_F(XmlValidatorTest, ReturnsValidForExistingA2A3AndA4Fixtures) {
    const std::pair<std::filesystem::path, std::filesystem::path> cases[] = {
        {"tests/xml/valid/A2_valid_simple_ok.xml", "tests/xsd/setting_check.xsd"},
        {"tests/xml/valid/A3_valid_simple_with_step.xml", "tests/xsd/mnt_rpt.xsd"},
        {"tests/xml/valid/A4_valid_simple_with_detail.xml", "tests/xsd/check_report.xsd"},
    };

    XmlValidator validator(runtime());
    for (const auto& [xmlRelativePath, xsdRelativePath] : cases) {
        const ValidationResult result = validator.validate(
            fixturePath(xmlRelativePath), fixturePath(xsdRelativePath));

        EXPECT_EQ(result.status, ValidationStatus::Valid) << xmlRelativePath;
        EXPECT_TRUE(result.errors.empty()) << xmlRelativePath;
    }
}

TEST_F(XmlValidatorTest, ReturnsInvalidWithLocationsForSchemaConstraintErrors) {
    const std::pair<std::filesystem::path, std::filesystem::path> cases[] = {
        {"tests/xml/invalid/A2_missing_appname_attr.xml", "tests/xsd/setting_check.xsd"},
        {"tests/xml/invalid/A3_invalid_total_estimate_value.xml", "tests/xsd/mnt_rpt.xsd"},
        {"tests/xml/invalid/A4_ied_item_missing_DeviceId.xml", "tests/xsd/check_report.xsd"},
    };

    XmlValidator validator(runtime());
    for (const auto& [xmlRelativePath, xsdRelativePath] : cases) {
        const ValidationResult result = validator.validate(
            fixturePath(xmlRelativePath), fixturePath(xsdRelativePath));

        ASSERT_EQ(result.status, ValidationStatus::Invalid) << xmlRelativePath;
        ASSERT_FALSE(result.errors.empty()) << xmlRelativePath;
        EXPECT_TRUE(std::any_of(result.errors.begin(), result.errors.end(), [](const ValidationError& error) {
            return error.severity != ErrorSeverity::Warning && error.line > 0 && error.column > 0;
        })) << xmlRelativePath;
    }
}

TEST_F(XmlValidatorTest, PreservesMultipleXmlErrors) {
    XmlValidator validator(runtime());
    const ValidationResult result = validator.validate(
        fixturePath("tests/xml/invalid/A2_multiple_errors.xml"),
        fixturePath("tests/xsd/setting_check.xsd"));

    ASSERT_EQ(result.status, ValidationStatus::Invalid);
    ASSERT_GE(result.errors.size(), 2U);
    EXPECT_TRUE(std::any_of(result.errors.begin(), result.errors.end(), [](const ValidationError& error) {
        return error.severity != ErrorSeverity::Warning && error.line > 0 && error.column > 0;
    }));
}

TEST_F(XmlValidatorTest, ReturnsInvalidForMalformedXml) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:element name=\"root\" type=\"xs:string\"/></xs:schema>");
    writeFile(xmlPath, "<root>unclosed</root");

    XmlValidator validator(runtime());
    const ValidationResult result = validator.validate(xmlPath, xsdPath);

    ASSERT_EQ(result.status, ValidationStatus::Invalid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(std::any_of(result.errors.begin(), result.errors.end(), [](const ValidationError& error) {
        return error.severity == ErrorSeverity::Fatal && error.line > 0 && error.column > 0;
    }));
}

TEST_F(XmlValidatorTest, ReturnsValidForTargetNamespaceSchema) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?>"
              "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" "
              "targetNamespace=\"urn:simple-xml-validator:test\" "
              "xmlns:tns=\"urn:simple-xml-validator:test\" elementFormDefault=\"qualified\">"
              "<xs:element name=\"root\" type=\"xs:string\"/>"
              "</xs:schema>");
    writeFile(xmlPath, "<root xmlns=\"urn:simple-xml-validator:test\">value</root>");

    XmlValidator validator(runtime());
    const ValidationResult result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.status, ValidationStatus::Valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(XmlValidatorTest, ReturnsFailedWithoutXmlErrorsForInvalidSchema) {
    const auto xsdPath = temporaryDirectory_ / "schema.xsd";
    const auto xmlPath = temporaryDirectory_ / "document.xml";
    writeFile(xsdPath,
              "<?xml version=\"1.0\"?><xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
              "<xs:element name=\"root\"></xs:schema>");
    writeFile(xmlPath, "<root/>");

    XmlValidator validator(runtime());
    const ValidationResult result = validator.validate(xmlPath, xsdPath);

    EXPECT_EQ(result.status, ValidationStatus::Failed);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_NE(result.message.find("XSD Schema 加载失败"), std::string::npos);
}

TEST_F(XmlValidatorTest, ReturnsFailedForMissingInput) {
    XmlValidator validator(runtime());
    const ValidationResult result = validator.validate(
        temporaryDirectory_ / "missing.xml", fixturePath("tests/xsd/setting_check.xsd"));

    EXPECT_EQ(result.status, ValidationStatus::Failed);
    EXPECT_TRUE(result.errors.empty());
    EXPECT_NE(result.message.find("XML 输入检查失败"), std::string::npos);
}

}
