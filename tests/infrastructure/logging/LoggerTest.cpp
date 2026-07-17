#include "core/validation/ValidationResult.h"
#include "infrastructure/logging/Logger.h"
#include "infrastructure/logging/ValidationLog.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace logging_ns = simple_xml_validator::infrastructure::logging;

namespace {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temporaryDirectory_ = std::filesystem::temp_directory_path()
                              / ("simple_xml_validator_logger_" + std::to_string(timestamp));
        std::filesystem::create_directories(temporaryDirectory_);
    }

    void TearDown() override {
        std::error_code errorCode;
        std::filesystem::remove_all(temporaryDirectory_, errorCode);
    }

    static std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        EXPECT_TRUE(input.is_open());
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    static std::vector<std::filesystem::path> rotatedFiles(
        const std::filesystem::path& directory,
        const std::filesystem::path& activeFile,
        const std::filesystem::path& otherActiveFile) {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file() || entry.path() == activeFile
                || entry.path() == otherActiveFile) {
                continue;
            }
            files.push_back(entry.path());
        }
        return files;
    }

    std::filesystem::path temporaryDirectory_;
};

TEST_F(LoggerTest, WritesModulesToTheirSelectedChannels) {
    logging_ns::LogConfig config;
    config.directory = temporaryDirectory_;
    logging_ns::LogManager manager(config);

    const logging_ns::LogModule systemModule =
        manager.module("SystemModule", logging_ns::LogChannel::System);
    const logging_ns::LogModule validationModule =
        manager.module("ValidationModule", logging_ns::LogChannel::Validation);

    ASSERT_TRUE(systemModule.ready());
    ASSERT_TRUE(validationModule.ready());
    systemModule.warning("system-message");
    validationModule.error("validation-message");

    const std::string systemContent = readFile(temporaryDirectory_ / config.systemLogFileName);
    const std::string validationContent =
        readFile(temporaryDirectory_ / config.validationLogFileName);

    EXPECT_NE(systemContent.find("SystemModule"), std::string::npos);
    EXPECT_NE(systemContent.find("warning"), std::string::npos);
    EXPECT_NE(systemContent.find("system-message"), std::string::npos);
    EXPECT_EQ(systemContent.find("validation-message"), std::string::npos);
    EXPECT_NE(validationContent.find("ValidationModule"), std::string::npos);
    EXPECT_NE(validationContent.find("error"), std::string::npos);
    EXPECT_NE(validationContent.find("validation-message"), std::string::npos);
    EXPECT_EQ(validationContent.find("system-message"), std::string::npos);
}

TEST_F(LoggerTest, RecordsCompleteValidationResultWithUtf8Paths) {
    logging_ns::LogConfig config;
    config.directory = temporaryDirectory_;
    logging_ns::LogManager manager(config);
    const logging_ns::LogModule validationModule =
        manager.module("ValidationRecord", logging_ns::LogChannel::Validation);

    const ValidationResult result{
        ValidationStatus::Invalid,
        {{ErrorSeverity::Warning, 3, 4, "提示信息"},
         {ErrorSeverity::Error, 12, 8, "属性无效"},
         {ErrorSeverity::Fatal, 20, 1, "文档格式错误"}},
        "XML 未通过校验"};

    logging_ns::logValidationRecord(
        validationModule,
        temporaryDirectory_ / std::filesystem::u8path(u8"含 空格.xml"),
        temporaryDirectory_ / std::filesystem::u8path(u8"规则.xsd"),
        result);

    const std::string content = readFile(temporaryDirectory_ / config.validationLogFileName);
    EXPECT_NE(content.find("含 空格.xml"), std::string::npos);
    EXPECT_NE(content.find("规则.xsd"), std::string::npos);
    EXPECT_NE(content.find("校验状态: Invalid"), std::string::npos);
    EXPECT_NE(content.find("错误数量: 3"), std::string::npos);
    EXPECT_NE(content.find("XML 未通过校验"), std::string::npos);
    EXPECT_NE(content.find("[Warning] 行 3 列 4 : 提示信息"), std::string::npos);
    EXPECT_NE(content.find("[Error] 行 12 列 8 : 属性无效"), std::string::npos);
    EXPECT_NE(content.find("[Fatal] 行 20 列 1 : 文档格式错误"), std::string::npos);
}

TEST_F(LoggerTest, KeepsSystemChannelWhenValidationTargetIsUnavailable) {
    const std::filesystem::path validationTarget = temporaryDirectory_ / "validation-target";
    std::filesystem::create_directories(validationTarget);

    logging_ns::LogConfig config;
    config.directory             = temporaryDirectory_;
    config.validationLogFileName = validationTarget.filename().string();
    logging_ns::LogManager manager(config);

    EXPECT_TRUE(manager.channelReady(logging_ns::LogChannel::System));
    EXPECT_FALSE(manager.channelReady(logging_ns::LogChannel::Validation));

    const logging_ns::LogModule systemModule =
        manager.module("SystemModule", logging_ns::LogChannel::System);
    const logging_ns::LogModule validationModule =
        manager.module("ValidationModule", logging_ns::LogChannel::Validation);
    ASSERT_TRUE(systemModule.ready());
    EXPECT_FALSE(validationModule.ready());

    EXPECT_NO_THROW(systemModule.info("system-still-available"));
    EXPECT_NO_THROW(validationModule.info("validation-is-disabled"));

    const std::string systemContent = readFile(temporaryDirectory_ / config.systemLogFileName);
    EXPECT_NE(systemContent.find("system-still-available"), std::string::npos);
}

TEST_F(LoggerTest, DegradesToNoOpWhenLogDirectoryIsInvalid) {
    const std::filesystem::path filePath = temporaryDirectory_ / "not-a-directory";
    std::ofstream file(filePath, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << "content";
    file.close();

    logging_ns::LogConfig config;
    config.directory = filePath / "child";
    logging_ns::LogManager manager(config);

    EXPECT_FALSE(manager.channelReady(logging_ns::LogChannel::System));
    EXPECT_FALSE(manager.channelReady(logging_ns::LogChannel::Validation));

    const std::string longModuleName(256, 'm');
    const std::string longMessage(4096, 'x');
    EXPECT_NO_THROW({
        const logging_ns::LogModule systemModule =
            manager.module(longModuleName, logging_ns::LogChannel::System);
        const logging_ns::LogModule validationModule =
            manager.module(longModuleName, logging_ns::LogChannel::Validation);
        systemModule.error(longMessage);
        logging_ns::logValidationRecord(
            validationModule,
            temporaryDirectory_ / "document.xml",
            temporaryDirectory_ / "schema.xsd",
            {ValidationStatus::Failed, {}, "failure"});
    });
}

TEST_F(LoggerTest, WritesLongModuleNameAndMessage) {
    logging_ns::LogConfig config;
    config.directory = temporaryDirectory_;
    logging_ns::LogManager manager(config);

    const std::string longModuleName(256, 'm');
    const std::string longMessage(4096, 'x');
    const logging_ns::LogModule module =
        manager.module(longModuleName, logging_ns::LogChannel::System);

    ASSERT_TRUE(module.ready());
    module.info(longMessage);

    const std::string content = readFile(temporaryDirectory_ / config.systemLogFileName);
    EXPECT_NE(content.find(longModuleName), std::string::npos);
    EXPECT_NE(content.find(longMessage), std::string::npos);
}

TEST_F(LoggerTest, RotatesBothChannelsAndRetainsLatestMessages) {
    logging_ns::LogConfig config;
    config.directory          = temporaryDirectory_;
    config.systemLogFileName  = "system_rotation.log";
    config.validationLogFileName = "validation_rotation.log";
    config.rotating         = true;
    config.maxFileSizeBytes = 1024;
    config.maxFiles         = 2;
    logging_ns::LogManager manager(config);

    const logging_ns::LogModule systemModule =
        manager.module("SystemRotation", logging_ns::LogChannel::System);
    const logging_ns::LogModule validationModule =
        manager.module("ValidationRotation", logging_ns::LogChannel::Validation);
    ASSERT_TRUE(systemModule.ready());
    ASSERT_TRUE(validationModule.ready());

    const std::string systemMessage(200, 's');
    const std::string validationMessage(200, 'v');
    for (int i = 0; i < 200; ++i) {
        systemModule.info(systemMessage);
        validationModule.info(validationMessage);
    }
    const std::string systemFinalMessage = "system-rotation-final";
    const std::string validationFinalMessage = "validation-rotation-final";
    systemModule.info(systemFinalMessage);
    validationModule.info(validationFinalMessage);

    const std::filesystem::path systemActiveFile =
        temporaryDirectory_ / config.systemLogFileName;
    const std::filesystem::path validationActiveFile =
        temporaryDirectory_ / config.validationLogFileName;
    ASSERT_TRUE(std::filesystem::exists(systemActiveFile));
    ASSERT_TRUE(std::filesystem::exists(validationActiveFile));
    EXPECT_NE(readFile(systemActiveFile).find(systemFinalMessage), std::string::npos);
    EXPECT_NE(readFile(validationActiveFile).find(validationFinalMessage), std::string::npos);

    const std::vector<std::filesystem::path> files =
        rotatedFiles(temporaryDirectory_, systemActiveFile, validationActiveFile);
    std::size_t systemRotatedFiles     = 0;
    std::size_t validationRotatedFiles = 0;
    for (const std::filesystem::path& file : files) {
        const std::string content = readFile(file);
        if (content.find(systemMessage) != std::string::npos) {
            ++systemRotatedFiles;
        }
        if (content.find(validationMessage) != std::string::npos) {
            ++validationRotatedFiles;
        }
    }
    EXPECT_EQ(systemRotatedFiles, config.maxFiles);
    EXPECT_EQ(validationRotatedFiles, config.maxFiles);
}

TEST_F(LoggerTest, RotatingConfigToleratesZeroLimits) {
    logging_ns::LogConfig config;
    config.directory        = temporaryDirectory_;
    config.rotating         = true;
    config.maxFileSizeBytes = 0;  // 非法值，应被内部下限保护
    config.maxFiles         = 0;
    logging_ns::LogManager manager(config);

    EXPECT_TRUE(manager.channelReady(logging_ns::LogChannel::System));
    EXPECT_TRUE(manager.channelReady(logging_ns::LogChannel::Validation));

    const logging_ns::LogModule module =
        manager.module("RotatingModule", logging_ns::LogChannel::System);
    ASSERT_TRUE(module.ready());
    module.info("zero-limit-first-message");
    module.info("zero-limit-final-message");

    const std::filesystem::path systemActiveFile =
        temporaryDirectory_ / config.systemLogFileName;
    const std::filesystem::path validationActiveFile =
        temporaryDirectory_ / config.validationLogFileName;
    ASSERT_TRUE(std::filesystem::exists(systemActiveFile));
    EXPECT_NE(readFile(systemActiveFile).find("zero-limit-final-message"), std::string::npos);

    const std::vector<std::filesystem::path> files =
        rotatedFiles(temporaryDirectory_, systemActiveFile, validationActiveFile);
    EXPECT_EQ(files.size(), 1u);
}

}  // namespace
