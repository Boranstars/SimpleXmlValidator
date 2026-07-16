#include "gui/ValidationResultPresenter.h"

#include <string>
#include <unordered_map>
#include <utility>

namespace simple_xml_validator::gui {
namespace {

// 严重级别到界面文本的稳定映射，与错误表格“级别”列一致。
std::string severityText(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Warning:
            return "Warning";
        case ErrorSeverity::Error:
            return "Error";
        case ErrorSeverity::Fatal:
            return "Fatal";
    }
    return "Unknown";
}

// 完全相同项的合并键：级别 + 行号 + 列号 + 描述。
std::string mergeKey(const std::string& severity, const ValidationError& error) {
    return severity + '\n' + std::to_string(error.line) + '\n' +
           std::to_string(error.column) + '\n' + error.message;
}

}  // namespace

PresentedResult ValidationResultPresenter::initial() const {
    // 初始状态：不显示提示条、错误数量、表格或弹窗。
    return PresentedResult{};
}

PresentedResult ValidationResultPresenter::present(const ValidationResult& result) const {
    PresentedResult presented;

    // 错误/警告计数对所有已完成校验都可用（Valid 也可能带警告）。
    for (const ValidationError& error : result.errors) {
        if (error.severity == ErrorSeverity::Warning) {
            ++presented.warningCount;
        } else {
            ++presented.errorCount;
        }
    }

    switch (result.status) {
        case ValidationStatus::Valid:
            // 绿色通过提示，不显示错误表格。
            presented.banner         = BannerKind::Success;
            presented.bannerText     = "XML 通过校验";
            presented.statusCard     = StatusCardKind::Valid;
            presented.statusCardText = "校验通过";
            break;

        case ValidationStatus::Invalid: {
            // 红色未通过提示 + 错误总数 + 可滚动表格；不使用弹窗打断用户。
            presented.banner          = BannerKind::Failure;
            presented.bannerText      = "XML 未通过校验";
            presented.statusCard      = StatusCardKind::Invalid;
            presented.statusCardText  = "校验未通过";
            presented.showErrorTable  = true;
            presented.totalErrorCount = result.errors.size();

            // 按首次出现顺序合并 (级别,行号,列号,描述) 完全相同的诊断并计数。
            // 合并只影响展示；totalErrorCount 与日志仍保留真实条数。
            std::unordered_map<std::string, std::size_t> indexByKey;
            for (const ValidationError& error : result.errors) {
                const std::string severity = severityText(error.severity);
                const std::string key      = mergeKey(severity, error);

                const auto found = indexByKey.find(key);
                if (found != indexByKey.end()) {
                    ++presented.errorRows[found->second].occurrences;
                    continue;
                }
                if (presented.errorRows.size() >= kMaxErrorRows) {
                    presented.truncated = true;
                    continue;
                }
                indexByKey.emplace(key, presented.errorRows.size());
                presented.errorRows.push_back(PresentedError{
                    severity,
                    std::to_string(error.line),
                    std::to_string(error.column),
                    error.message,
                    1});
            }
            break;
        }

        case ValidationStatus::Failed:
            // 阻断性问题：清理普通结果区域，改用明确的阻断性弹窗提示。
            presented.statusCard         = StatusCardKind::Failed;
            presented.statusCardText     = "校验失败";
            presented.showBlockingDialog = true;
            presented.dialogMessage =
                result.message.empty() ? "校验无法完成，请检查输入文件或稍后重试。"
                                       : result.message;
            break;
    }

    return presented;
}

}  // namespace simple_xml_validator::gui

