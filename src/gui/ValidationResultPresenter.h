#ifndef SIMPLE_XML_VALIDATOR_GUI_VALIDATION_RESULT_PRESENTER_H
#define SIMPLE_XML_VALIDATOR_GUI_VALIDATION_RESULT_PRESENTER_H

#include "core/validation/ValidationResult.h"

#include <cstddef>
#include <string>
#include <vector>

namespace simple_xml_validator::gui {

// 结果提示条类型。用于驱动主窗口提示条的显示与配色，
// 不在此处耦合具体 Qt 颜色，仅表达语义。
enum class BannerKind {
    None,     // 初始状态或重新选择文件后：不显示提示条
    Success,  // 绿色“XML 通过校验”
    Failure   // 红色“XML 未通过校验”
};

// 单条错误在表格中的展示形态。所有字段均为可直接渲染的字符串，
// 行号/列号为 0 时表示位置未知，保持为 "0"，不伪造位置。
// occurrences 为该 (级别,行号,列号,描述) 完全相同项被合并的次数（>=1）；
// Xerces 将属性级错误定位到所属元素的行列，同一元素多个属性违规会产生
// 文本完全相同的诊断，这里在展示层合并计数，日志仍保留每条原始明细。
struct PresentedError {
    std::string severity;
    std::string line;
    std::string column;
    std::string message;
    std::size_t occurrences = 1;
};

// 由 ValidationResult 映射得到的稳定展示状态。主窗口据此更新控件，
// 使映射逻辑可脱离 Qt 由 GTest 独立验证。
struct PresentedResult {
    BannerKind  banner    = BannerKind::None;
    std::string bannerText;

    bool                        showErrorTable  = false;
    std::size_t                 totalErrorCount = 0;   // 错误总数（合并前的真实条数）
    std::vector<PresentedError> errorRows;             // 合并后的错误行（受上限约束）
    bool                        truncated       = false;  // 是否因上限截断错误“类别”

    bool        showBlockingDialog = false;  // Failed 等阻断问题使用弹窗
    std::string dialogMessage;
};

// 将 ValidationResult 映射为主窗口展示状态。不创建、不持有 Qt 控件，
// 不依赖 Xerces；便于 GTest 独立验证映射规则。
class ValidationResultPresenter {
public:
    // 单次展示的最大错误“类别”行数（合并后）。避免大量诊断无界渲染导致界面
    // 卡顿；完整诊断仍由 ValidationResult 与日志保留。
    static constexpr std::size_t kMaxErrorRows = 200;

    // 初始状态或重新选择文件后的空展示状态。
    [[nodiscard]] PresentedResult initial() const;

    // 依据校验结果映射展示状态。
    [[nodiscard]] PresentedResult present(const ValidationResult& result) const;
};

}  // namespace simple_xml_validator::gui

#endif
