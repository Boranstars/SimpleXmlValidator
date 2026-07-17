#include "core/validation/InputValidator.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class CurrentPathGuard {
public:
    explicit CurrentPathGuard(std::filesystem::path originalPath)
        : originalPath_(std::move(originalPath)) {
    }

    ~CurrentPathGuard() {
        std::error_code errorCode;
        std::filesystem::current_path(originalPath_, errorCode);
    }

private:
    std::filesystem::path originalPath_;
};

class InputValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temporaryDirectory_ = std::filesystem::temp_directory_path()
                              / ("simple_xml_validator_input_validator_" + std::to_string(timestamp));
        std::filesystem::create_directories(temporaryDirectory_);
    }

    void TearDown() override {
        std::error_code errorCode;
        std::filesystem::permissions(
            temporaryDirectory_,
            std::filesystem::perms::owner_all,
            std::filesystem::perm_options::add,
            errorCode);
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


TEST_F(InputValidatorTest, RejectsEmptyPath) {
    const auto result = InputValidator::check({});

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
    EXPECT_EQ(result.errorMessage, "路径为空");
}

TEST_F(InputValidatorTest, AcceptsExistingRelativeFileAndReturnsAbsolutePath) {
    // Change CWD to temp dir so relative() stays on the same drive (required on Windows
    // where the runner CWD may be on a different drive than TEMP).
    CurrentPathGuard guard(std::filesystem::current_path());
    std::filesystem::current_path(temporaryDirectory_);

    const auto absolutePath = temporaryDirectory_ / "relative-input.xml";
    writeFile(absolutePath, "<root/>");
    const auto relativePath = std::filesystem::relative(absolutePath, std::filesystem::current_path());

    ASSERT_FALSE(relativePath.empty());
    ASSERT_FALSE(relativePath.is_absolute());

    const auto result = InputValidator::check(relativePath);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, std::filesystem::absolute(relativePath).lexically_normal());
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, AcceptsExistingAbsoluteFile) {
    const auto absolutePath = temporaryDirectory_ / "absolute-input.xml";
    writeFile(absolutePath, "<root/>");

    const auto result = InputValidator::check(absolutePath);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, absolutePath);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, AcceptsChineseAndSpacePath) {
    const auto path = temporaryDirectory_ / u8"包含 空格 的目录" / u8"有效 文件.xml";
    writeFile(path, "<root/>");

    const auto result = InputValidator::check(path);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, path);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, RejectsNonexistentFile) {
    const auto path = temporaryDirectory_ / std::filesystem::u8path(u8"中文 空格")
                                          / std::filesystem::u8path(u8"missing file.xml");

    const auto result = InputValidator::check(path);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
    EXPECT_NE(result.errorMessage.find("文件不存在"), std::string::npos);
    EXPECT_NE(result.errorMessage.find(u8"中文 空格"), std::string::npos);
}

TEST_F(InputValidatorTest, RejectsDirectory) {
    const auto directoryPath = temporaryDirectory_ / "directory-input";
    std::filesystem::create_directories(directoryPath);

    const auto result = InputValidator::check(directoryPath);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
    EXPECT_NE(result.errorMessage.find("路径不是普通文件"), std::string::npos);
}

TEST_F(InputValidatorTest, RejectsEmptyFile) {
    const auto path = temporaryDirectory_ / "empty-input.xml";
    writeFile(path, "");

    const auto result = InputValidator::check(path);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
    EXPECT_NE(result.errorMessage.find("文件为空"), std::string::npos);
}

TEST_F(InputValidatorTest, AcceptsFileWithUnexpectedExtension) {
    const auto path = temporaryDirectory_ / "input.txt";
    writeFile(path, "content");

    const auto result = InputValidator::check(path);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, path);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, NormalizesParentDirectorySegments) {
    const auto expectedPath = temporaryDirectory_ / "normalized-input.xml";
    writeFile(expectedPath, "<root/>");
    const auto inputPath = temporaryDirectory_ / "nested" / ".." / "normalized-input.xml";

    const auto result = InputValidator::check(inputPath);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, expectedPath);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, NormalizesRepeatedSeparators) {
    const auto expectedPath = temporaryDirectory_ / "repeated-separators.xml";
    writeFile(expectedPath, "<root/>");
    const auto inputPath = std::filesystem::path(
        temporaryDirectory_.generic_u8string() + "///repeated-separators.xml");

    const auto result = InputValidator::check(inputPath);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, expectedPath);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, AcceptsFileNamedOnlyWithExtension) {
    const auto path = temporaryDirectory_ / ".xml";
    writeFile(path, "<root/>");

    const auto result = InputValidator::check(path);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, path);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, AcceptsCrossPlatformSafeSpecialCharacters) {
    const auto path = temporaryDirectory_ / "special-@#$%[]().xml";
    writeFile(path, "<root/>");

    const auto result = InputValidator::check(path);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, path);
    EXPECT_TRUE(result.errorMessage.empty());
}

TEST_F(InputValidatorTest, ResolvesRelativePathFromCurrentWorkingDirectory) {
    const auto workingDirectory = temporaryDirectory_ / "working-directory";
    writeFile(workingDirectory / "inputs" / "current-directory.xml", "<root/>");

    CurrentPathGuard guard(std::filesystem::current_path());
    std::filesystem::current_path(workingDirectory);

    const auto inputPath = std::filesystem::path("inputs") / "." / "current-directory.xml";
    // Compute expectedPath after cwd change so getcwd()-based resolution is consistent
    // with the implementation on macOS (where /var is a symlink to /private/var).
    const auto expectedPath = std::filesystem::absolute(inputPath).lexically_normal();
    const auto result = InputValidator::check(inputPath);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.absolutePath, expectedPath);
    EXPECT_TRUE(result.errorMessage.empty());
}

#ifndef _WIN32
TEST_F(InputValidatorTest, RejectsRegularFilePathWithTrailingSeparator) {
    const auto path = temporaryDirectory_ / "trailing-separator.xml";
    writeFile(path, "<root/>");
    const auto inputPath = std::filesystem::path(path.generic_u8string() + "/");

    const auto result = InputValidator::check(inputPath);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
}
#endif

#ifndef _WIN32
TEST_F(InputValidatorTest, RejectsUnreadableFileWhenPermissionsAreEnforced) {
    const auto path = temporaryDirectory_ / "unreadable-input.xml";
    writeFile(path, "<root/>");

    std::error_code errorCode;
    std::filesystem::permissions(
        path,
        std::filesystem::perms::none,
        std::filesystem::perm_options::replace,
        errorCode);
    ASSERT_FALSE(errorCode);

    std::ifstream input(path);
    if (input.is_open()) {
        GTEST_SKIP() << "当前运行身份仍可读取无权限文件";
    }

    const auto result = InputValidator::check(path);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.absolutePath.empty());
    EXPECT_NE(result.errorMessage.find("文件无法读取"), std::string::npos);
}
#endif

}
