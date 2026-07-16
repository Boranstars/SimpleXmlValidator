#include "../../infrastructure/xerces/XercesTestFixture.h"

#include "core/validation/XmlValidator.h"
#include "infrastructure/logging/Logger.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace logging_ns = simple_xml_validator::infrastructure::logging;

namespace {

struct XmlFixtureCase {
    const char* name;
    const char* xmlRelativePath;
    const char* xsdRelativePath;
};

void PrintTo(const XmlFixtureCase& testCase, std::ostream* output) {
    *output << testCase.name;
}

class XmlValidatorFixture : public XercesTestFixture {
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

    static std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        EXPECT_TRUE(input.is_open());
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    static void expectEquivalentResult(
        const ValidationResult& expected,
        const ValidationResult& actual) {
        EXPECT_EQ(actual.status, expected.status);
        EXPECT_EQ(actual.message, expected.message);
        ASSERT_EQ(actual.errors.size(), expected.errors.size());
        for (std::size_t index = 0; index < expected.errors.size(); ++index) {
            EXPECT_EQ(actual.errors[index].severity, expected.errors[index].severity);
            EXPECT_EQ(actual.errors[index].line, expected.errors[index].line);
            EXPECT_EQ(actual.errors[index].column, expected.errors[index].column);
            EXPECT_EQ(actual.errors[index].message, expected.errors[index].message);
        }
    }

    std::filesystem::path fixturePath(const std::filesystem::path& relativePath) const {
        return std::filesystem::path(SIMPLE_XML_VALIDATOR_SOURCE_ROOT) / relativePath;
    }

    static bool hasLocatedBlockingError(const ValidationResult& result) {
        return std::any_of(result.errors.begin(), result.errors.end(), [](const ValidationError& error) {
            return error.severity != ErrorSeverity::Warning && error.line > 0 && error.column > 0;
        });
    }

    static bool hasErrorMessageToken(const ValidationResult& result, const std::string& token) {
        return std::any_of(result.errors.begin(), result.errors.end(), [&token](const ValidationError& error) {
            return error.severity != ErrorSeverity::Warning
                   && error.message.find(token) != std::string::npos;
        });
    }

    std::filesystem::path temporaryDirectory_;
};

class XmlValidatorTest : public XmlValidatorFixture {
};

class ValidXmlFixtureTest : public XmlValidatorFixture,
                            public ::testing::WithParamInterface<XmlFixtureCase> {
};

class InvalidXmlFixtureTest : public XmlValidatorFixture,
                              public ::testing::WithParamInterface<XmlFixtureCase> {
};

std::string fixtureCaseName(const ::testing::TestParamInfo<XmlFixtureCase>& info) {
    return info.param.name;
}

TEST_P(ValidXmlFixtureTest, ReturnsValidWithoutErrors) {
    const XmlFixtureCase& testCase = GetParam();
    XmlValidator validator(runtime());

    const ValidationResult result = validator.validate(
        fixturePath(testCase.xmlRelativePath), fixturePath(testCase.xsdRelativePath));

    EXPECT_EQ(result.status, ValidationStatus::Valid);
    EXPECT_TRUE(result.errors.empty());
}

INSTANTIATE_TEST_SUITE_P(
    AllValidFixtures,
    ValidXmlFixtureTest,
    ::testing::Values(
        XmlFixtureCase{"A2ValidSimpleOk", "tests/xml/valid/A2_valid_simple_ok.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2ValidWithZone", "tests/xml/valid/A2_valid_with_zone.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A3ValidSimpleWithStep", "tests/xml/valid/A3_valid_simple_with_step.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3ValidFullWithSteps", "tests/xml/valid/A3_valid_full_with_steps.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A4ValidSimpleWithDetail", "tests/xml/valid/A4_valid_simple_with_detail.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4ValidMultiSections", "tests/xml/valid/A4_valid_multi_sections.xml", "tests/xsd/check_report.xsd"}),
    fixtureCaseName);

TEST_P(InvalidXmlFixtureTest, ReturnsLocatedInvalidDiagnostics) {
    const XmlFixtureCase& testCase = GetParam();
    XmlValidator validator(runtime());

    const ValidationResult result = validator.validate(
        fixturePath(testCase.xmlRelativePath), fixturePath(testCase.xsdRelativePath));

    ASSERT_EQ(result.status, ValidationStatus::Invalid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(hasLocatedBlockingError(result));
}

INSTANTIATE_TEST_SUITE_P(
    AllInvalidFixtures,
    InvalidXmlFixtureTest,
    ::testing::Values(
        XmlFixtureCase{"A2InvalidTimeFormat", "tests/xml/invalid/A2_invalid_time_format.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2MissingAppnameAttr", "tests/xml/invalid/A2_missing_appname_attr.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2MissingCompareTemplate", "tests/xml/invalid/A2_missing_CompareTemplate.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2MissingQueryTimeElement", "tests/xml/invalid/A2_missing_QueryTime_element.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2MissingResultElement", "tests/xml/invalid/A2_missing_Result_element.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2MultipleErrors", "tests/xml/invalid/A2_multiple_errors.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2PointMissingOtherAttr", "tests/xml/invalid/A2_point_missing_other_attr.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2ZoneInvalidIsDifferent", "tests/xml/invalid/A2_zone_invalid_IsDifferent.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A2ZoneMissingValue", "tests/xml/invalid/A2_zone_missing_Value.xml", "tests/xsd/setting_check.xsd"},
        XmlFixtureCase{"A3InvalidBaseTimeFormat", "tests/xml/invalid/A3_invalid_Base_time_format.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3InvalidTotalEstimateValue", "tests/xml/invalid/A3_invalid_total_estimate_value.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3MissingBaseElement", "tests/xml/invalid/A3_missing_Base_element.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3MissingIedElement", "tests/xml/invalid/A3_missing_Ied_element.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3MissingSubstationElement", "tests/xml/invalid/A3_missing_Substation_element.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3MultipleErrors", "tests/xml/invalid/A3_multiple_errors.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A3StepInvalidEstimate", "tests/xml/invalid/A3_step_invalid_estimate.xml", "tests/xsd/mnt_rpt.xsd"},
        XmlFixtureCase{"A4AlarmItemInvalidIsDifferent", "tests/xml/invalid/A4_alarm_item_invalid_IsDifferent.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4AlarmSectionMissingItem", "tests/xml/invalid/A4_alarm_section_missing_Item.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4IedItemMissingDeviceId", "tests/xml/invalid/A4_ied_item_missing_DeviceId.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4InvalidCheckTimeFormat", "tests/xml/invalid/A4_invalid_CheckTime_format.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4MissingIedSummary", "tests/xml/invalid/A4_missing_Ied_summary.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4MissingSystemElement", "tests/xml/invalid/A4_missing_System_element.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4MultipleErrors", "tests/xml/invalid/A4_multiple_errors.xml", "tests/xsd/check_report.xsd"},
        XmlFixtureCase{"A4SystemMissingCheckTime", "tests/xml/invalid/A4_system_missing_CheckTime.xml", "tests/xsd/check_report.xsd"}),
    fixtureCaseName);

TEST_F(XmlValidatorTest, ReportsExpectedNamesForRequiredElementOmissions) {
    const std::pair<XmlFixtureCase, std::string> cases[] = {
        {{"A2MissingQueryTimeElement", "tests/xml/invalid/A2_missing_QueryTime_element.xml", "tests/xsd/setting_check.xsd"}, "QueryTime"},
        {{"A3MissingBaseElement", "tests/xml/invalid/A3_missing_Base_element.xml", "tests/xsd/mnt_rpt.xsd"}, "Base"},
        {{"A4MissingSystemElement", "tests/xml/invalid/A4_missing_System_element.xml", "tests/xsd/check_report.xsd"}, "System"},
    };

    XmlValidator validator(runtime());
    for (const auto& [testCase, expectedToken] : cases) {
        const ValidationResult result = validator.validate(
            fixturePath(testCase.xmlRelativePath), fixturePath(testCase.xsdRelativePath));

        ASSERT_EQ(result.status, ValidationStatus::Invalid) << testCase.name;
        EXPECT_TRUE(hasLocatedBlockingError(result)) << testCase.name;
        EXPECT_TRUE(hasErrorMessageToken(result, expectedToken)) << testCase.name;
    }
}

TEST_F(XmlValidatorTest, ReportsExpectedNamesForRequiredAttributeOmissions) {
    const std::pair<XmlFixtureCase, std::string> cases[] = {
        {{"A2MissingAppnameAttr", "tests/xml/invalid/A2_missing_appname_attr.xml", "tests/xsd/setting_check.xsd"}, "appname"},
        {{"A4IedItemMissingDeviceId", "tests/xml/invalid/A4_ied_item_missing_DeviceId.xml", "tests/xsd/check_report.xsd"}, "DeviceId"},
    };

    XmlValidator validator(runtime());
    for (const auto& [testCase, expectedToken] : cases) {
        const ValidationResult result = validator.validate(
            fixturePath(testCase.xmlRelativePath), fixturePath(testCase.xsdRelativePath));

        ASSERT_EQ(result.status, ValidationStatus::Invalid) << testCase.name;
        EXPECT_TRUE(hasLocatedBlockingError(result)) << testCase.name;
        EXPECT_TRUE(hasErrorMessageToken(result, expectedToken)) << testCase.name;
    }
}

TEST_F(XmlValidatorTest, PreservesMultipleErrorsForAllSchemas) {
    const XmlFixtureCase cases[] = {
        {"A2MultipleErrors", "tests/xml/invalid/A2_multiple_errors.xml", "tests/xsd/setting_check.xsd"},
        {"A3MultipleErrors", "tests/xml/invalid/A3_multiple_errors.xml", "tests/xsd/mnt_rpt.xsd"},
        {"A4MultipleErrors", "tests/xml/invalid/A4_multiple_errors.xml", "tests/xsd/check_report.xsd"},
    };

    XmlValidator validator(runtime());
    for (const XmlFixtureCase& testCase : cases) {
        const ValidationResult result = validator.validate(
            fixturePath(testCase.xmlRelativePath), fixturePath(testCase.xsdRelativePath));

        ASSERT_EQ(result.status, ValidationStatus::Invalid) << testCase.name;
        EXPECT_GE(result.errors.size(), 2U) << testCase.name;
    }
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

TEST_F(XmlValidatorTest, LogsValidResultToBothChannels) {
    const auto logDirectory = temporaryDirectory_ / "logs";
    logging_ns::LogConfig config;
    config.directory = logDirectory;
    logging_ns::LogManager logManager(config);
    XmlValidator validator(runtime(), &logManager);

    const ValidationResult result = validator.validate(
        fixturePath("tests/xml/valid/A2_valid_simple_ok.xml"),
        fixturePath("tests/xsd/setting_check.xsd"));

    ASSERT_EQ(result.status, ValidationStatus::Valid);
    const std::string systemContent = readFile(logDirectory / config.systemLogFileName);
    const std::string validationContent =
        readFile(logDirectory / config.validationLogFileName);
    EXPECT_NE(systemContent.find("开始校验"), std::string::npos);
    EXPECT_NE(systemContent.find("校验结束，状态=Valid，错误数量=0"), std::string::npos);
    EXPECT_NE(validationContent.find("校验状态: Valid"), std::string::npos);
    EXPECT_NE(validationContent.find("错误数量: 0"), std::string::npos);
    EXPECT_NE(validationContent.find("A2_valid_simple_ok.xml"), std::string::npos);
}

TEST_F(XmlValidatorTest, LogsInvalidResultDetailsOnlyToValidationChannel) {
    const auto logDirectory = temporaryDirectory_ / "logs";
    logging_ns::LogConfig config;
    config.directory = logDirectory;
    logging_ns::LogManager logManager(config);
    XmlValidator validator(runtime(), &logManager);

    const ValidationResult result = validator.validate(
        fixturePath("tests/xml/invalid/A2_missing_appname_attr.xml"),
        fixturePath("tests/xsd/setting_check.xsd"));

    ASSERT_EQ(result.status, ValidationStatus::Invalid);
    ASSERT_FALSE(result.errors.empty());
    const std::string systemContent = readFile(logDirectory / config.systemLogFileName);
    const std::string validationContent =
        readFile(logDirectory / config.validationLogFileName);
    EXPECT_NE(systemContent.find("校验结束，状态=Invalid"), std::string::npos);
    EXPECT_EQ(systemContent.find(result.errors.front().message), std::string::npos);
    EXPECT_NE(validationContent.find("校验状态: Invalid"), std::string::npos);
    EXPECT_NE(validationContent.find(result.errors.front().message), std::string::npos);
}

TEST_F(XmlValidatorTest, KeepsValidationResultWhenLoggingIsUnavailable) {
    logging_ns::LogConfig disabledConfig;
    const auto blockingPath = temporaryDirectory_ / "not-a-log-directory";
    writeFile(blockingPath, "content");
    disabledConfig.directory = blockingPath / "child";
    logging_ns::LogManager disabledLogger(disabledConfig);

    XmlValidator baselineValidator(runtime());
    XmlValidator loggedValidator(runtime(), &disabledLogger);
    const std::pair<std::filesystem::path, std::filesystem::path> cases[] = {
        {fixturePath("tests/xml/valid/A2_valid_simple_ok.xml"),
         fixturePath("tests/xsd/setting_check.xsd")},
        {fixturePath("tests/xml/invalid/A2_missing_appname_attr.xml"),
         fixturePath("tests/xsd/setting_check.xsd")},
        {temporaryDirectory_ / "missing.xml", fixturePath("tests/xsd/setting_check.xsd")},
    };

    for (const auto& testCase : cases) {
        const ValidationResult baselineResult =
            baselineValidator.validate(testCase.first, testCase.second);
        const ValidationResult loggedResult =
            loggedValidator.validate(testCase.first, testCase.second);
        expectEquivalentResult(baselineResult, loggedResult);
    }
}

}
