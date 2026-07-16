#include "gui/ValidationResultPresenter.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace {

using simple_xml_validator::gui::BannerKind;
using simple_xml_validator::gui::ValidationResultPresenter;

ValidationError makeError(
    ErrorSeverity severity,
    std::size_t line,
    std::size_t column,
    std::string message) {
    return {severity, line, column, std::move(message)};
}

TEST(ValidationResultPresenterTest, InitialStateHidesAllResultElements) {
    const ValidationResultPresenter presenter;

    const auto presented = presenter.initial();

    EXPECT_EQ(presented.banner, BannerKind::None);
    EXPECT_TRUE(presented.bannerText.empty());
    EXPECT_FALSE(presented.showErrorTable);
    EXPECT_EQ(presented.totalErrorCount, 0U);
    EXPECT_TRUE(presented.errorRows.empty());
    EXPECT_FALSE(presented.truncated);
    EXPECT_FALSE(presented.showBlockingDialog);
    EXPECT_TRUE(presented.dialogMessage.empty());
}

TEST(ValidationResultPresenterTest, ValidResultShowsSuccessWithoutErrorData) {
    const ValidationResultPresenter presenter;
    const ValidationResult result{
        ValidationStatus::Valid,
        {makeError(ErrorSeverity::Error, 4, 9, "ignored error")},
        "ignored message"};

    const auto presented = presenter.present(result);

    EXPECT_EQ(presented.banner, BannerKind::Success);
    EXPECT_EQ(presented.bannerText, "XML 通过校验");
    EXPECT_FALSE(presented.showErrorTable);
    EXPECT_EQ(presented.totalErrorCount, 0U);
    EXPECT_TRUE(presented.errorRows.empty());
    EXPECT_FALSE(presented.truncated);
    EXPECT_FALSE(presented.showBlockingDialog);
    EXPECT_TRUE(presented.dialogMessage.empty());
}

TEST(ValidationResultPresenterTest, InvalidResultMapsErrorsAndPreservesUnknownLocation) {
    const ValidationResultPresenter presenter;
    const ValidationResult result{
        ValidationStatus::Invalid,
        {
            makeError(ErrorSeverity::Warning, 0, 0, "warning message"),
            makeError(ErrorSeverity::Error, 7, 11, "error message"),
            makeError(ErrorSeverity::Fatal, 13, 17, "fatal message")
        },
        "validation details"};

    const auto presented = presenter.present(result);

    ASSERT_EQ(presented.banner, BannerKind::Failure);
    EXPECT_EQ(presented.bannerText, "XML 未通过校验");
    EXPECT_TRUE(presented.showErrorTable);
    EXPECT_EQ(presented.totalErrorCount, 3U);
    ASSERT_EQ(presented.errorRows.size(), 3U);
    EXPECT_EQ(presented.errorRows[0].severity, "Warning");
    EXPECT_EQ(presented.errorRows[0].line, "0");
    EXPECT_EQ(presented.errorRows[0].column, "0");
    EXPECT_EQ(presented.errorRows[0].message, "warning message");
    EXPECT_EQ(presented.errorRows[0].occurrences, 1U);
    EXPECT_EQ(presented.errorRows[1].severity, "Error");
    EXPECT_EQ(presented.errorRows[1].line, "7");
    EXPECT_EQ(presented.errorRows[1].column, "11");
    EXPECT_EQ(presented.errorRows[1].message, "error message");
    EXPECT_EQ(presented.errorRows[2].severity, "Fatal");
    EXPECT_EQ(presented.errorRows[2].line, "13");
    EXPECT_EQ(presented.errorRows[2].column, "17");
    EXPECT_EQ(presented.errorRows[2].message, "fatal message");
    EXPECT_FALSE(presented.truncated);
    EXPECT_FALSE(presented.showBlockingDialog);
}

TEST(ValidationResultPresenterTest, InvalidResultMergesOnlyIdenticalErrorsInFirstSeenOrder) {
    const ValidationResultPresenter presenter;
    const ValidationResult result{
        ValidationStatus::Invalid,
        {
            makeError(ErrorSeverity::Error, 3, 5, "duplicate"),
            makeError(ErrorSeverity::Error, 3, 5, "duplicate"),
            makeError(ErrorSeverity::Warning, 3, 5, "duplicate"),
            makeError(ErrorSeverity::Error, 3, 6, "duplicate"),
            makeError(ErrorSeverity::Error, 3, 5, "different")
        },
        {}};

    const auto presented = presenter.present(result);

    EXPECT_EQ(presented.totalErrorCount, 5U);
    ASSERT_EQ(presented.errorRows.size(), 4U);
    EXPECT_EQ(presented.errorRows[0].severity, "Error");
    EXPECT_EQ(presented.errorRows[0].line, "3");
    EXPECT_EQ(presented.errorRows[0].column, "5");
    EXPECT_EQ(presented.errorRows[0].message, "duplicate");
    EXPECT_EQ(presented.errorRows[0].occurrences, 2U);
    EXPECT_EQ(presented.errorRows[1].severity, "Warning");
    EXPECT_EQ(presented.errorRows[1].occurrences, 1U);
    EXPECT_EQ(presented.errorRows[2].column, "6");
    EXPECT_EQ(presented.errorRows[3].message, "different");
}

TEST(ValidationResultPresenterTest, InvalidResultLimitsRowsWhileCountingDisplayedDuplicates) {
    const ValidationResultPresenter presenter;
    std::vector<ValidationError> errors;
    errors.reserve(ValidationResultPresenter::kMaxErrorRows + 2U);
    for (std::size_t index = 0; index < ValidationResultPresenter::kMaxErrorRows; ++index) {
        errors.push_back(makeError(
            ErrorSeverity::Error,
            index + 1U,
            1U,
            "error-" + std::to_string(index)));
    }
    errors.push_back(makeError(ErrorSeverity::Error, 1, 1, "error-0"));
    errors.push_back(makeError(ErrorSeverity::Fatal, 999, 1, "overflow"));

    const auto presented = presenter.present({ValidationStatus::Invalid, std::move(errors), {}});

    EXPECT_EQ(presented.totalErrorCount, ValidationResultPresenter::kMaxErrorRows + 2U);
    EXPECT_EQ(presented.errorRows.size(), ValidationResultPresenter::kMaxErrorRows);
    EXPECT_EQ(presented.errorRows.front().occurrences, 2U);
    EXPECT_TRUE(presented.truncated);
    EXPECT_EQ(presented.errorRows.back().message,
              "error-" + std::to_string(ValidationResultPresenter::kMaxErrorRows - 1U));
}

TEST(ValidationResultPresenterTest, FailedResultUsesBlockingDialogAndDefaultMessageWhenNeeded) {
    const ValidationResultPresenter presenter;

    const auto customMessage = presenter.present(
        {ValidationStatus::Failed, {makeError(ErrorSeverity::Error, 1, 1, "ignored")}, "XSD 无法加载"});
    const auto defaultMessage = presenter.present({ValidationStatus::Failed, {}, {}});

    EXPECT_EQ(customMessage.banner, BannerKind::None);
    EXPECT_FALSE(customMessage.showErrorTable);
    EXPECT_EQ(customMessage.totalErrorCount, 0U);
    EXPECT_TRUE(customMessage.errorRows.empty());
    EXPECT_TRUE(customMessage.showBlockingDialog);
    EXPECT_EQ(customMessage.dialogMessage, "XSD 无法加载");
    EXPECT_TRUE(defaultMessage.showBlockingDialog);
    EXPECT_EQ(defaultMessage.dialogMessage, "校验无法完成，请检查输入文件或稍后重试。");
}

}
