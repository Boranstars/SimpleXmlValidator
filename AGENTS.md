# SimpleXmlValidator 开发指南

## 1. 文档优先级与项目目标

- `docs/概要设计报告.md` 是本项目实现的最高设计依据；其内容与其他需求、示例或历史实现冲突时，以该文档为准。
- `docs/XML语法校验工具-设计方案文档.md` 是上游需求参考，仅在不与概要设计冲突时使用。
- `docs/模块实施计划.md` 记录模块实施顺序与当前进度；开始工作前应先查看该文件，并在阶段状态变化后同步更新。
- 生产代码由 Claude 按计划实现；Codex 负责阶段实现检查、GTest 编写与验证，不直接实现计划中的生产代码。
- 项目第一版实现本地、单个 XML 文件与单个 XSD 文件的校验工具：检查 XML 格式良好性并执行 XSD Schema 校验。
- 目标用户为开发、测试和配置维护人员；优先保证结果准确、错误定位清晰、界面操作简单和程序稳定。

## 2. 技术基线

- 使用 C++17、Qt 5.12.x（`Core`、`Gui`、`Widgets`）、CMake、Xerces-C++、spdlog 和 GTest。
- 保持跨平台兼容性，目标平台为 Windows 10/11 与 Linux x86_64。
- 路径统一使用 `std::filesystem::path` 表达和处理；在进入校验流程前转换为绝对且词法规范化的路径（不解析符号链接），必须支持中文路径和含空格路径。
- 通过 CMake 管理依赖，不要在代码中硬编码第三方库、Qt 安装目录或平台专属路径。
- 新增源文件、测试和依赖时，同步更新 `CMakeLists.txt`，并沿用既有目标与链接方式。

## 3. 分层与模块职责

建议的目录职责如下；创建目录或文件时按此划分，避免将所有逻辑堆入 `main.cpp`：

```text
src/
  gui/          # Qt 窗口、控件、展示模型与用户交互
  core/         # 不依赖 Qt 的核心业务逻辑与公共结果模型
    validation/ # XmlValidator、输入检查、结果数据模型
  infrastructure/ # Xerces-C++ 封装、错误处理器、日志实现
tests/          # GTest 单元测试
```

- **GUI 层**：仅负责文件选择、按钮状态、结果展示和阻断性异常提示；不得直接调用 Xerces-C++ API，也不得解析 Xerces 原始异常对象。
- **输入检查模块**：负责路径为空、存在性、普通文件、可读性与绝对且词法规范化路径转换；不负责判断 XSD 内容是否合法。
- **XML 校验模块**：由 `XmlValidator` 组织 Xerces 初始化、Schema 加载、XML 解析、Schema 校验与结果封装。
- **错误收集模块**：实现 Xerces `ErrorHandler`，将 `warning`、`error`、`fatalError` 转换为项目内部错误结构。
- **结果模型模块**：仅描述校验结果，不依赖 Qt 控件或 Xerces 原始对象，供 GUI、日志和测试共同使用。
- **日志模块**：记录运行状态、关键操作和异常。日志失败不得中断或改变校验主流程。

## 4. 必须保持的核心接口与数据语义

除非同步修改 `docs/概要设计报告.md` 并有明确理由，不要改变以下对外语义：

```cpp
class XmlValidator {
public:
    ValidationResult validate(
        const std::filesystem::path& xmlPath,
        const std::filesystem::path& xsdPath);
};
```

```cpp
enum class ValidationStatus { Valid, Invalid, Failed };
enum class ErrorSeverity { Warning, Error, Fatal };

struct ValidationError {
    ErrorSeverity severity;
    std::size_t line;
    std::size_t column;
    std::string message;
};

struct ValidationResult {
    ValidationStatus status;
    std::vector<ValidationError> errors;
    std::string message;
};
```

- `Valid`：XML 格式良好且通过已成功加载的 XSD 校验；通常没有错误。
- `Invalid`：校验流程正常完成，但 XML 内容或结构不符合 XSD/格式要求；错误明细放入 `errors`，用于主界面表格展示。
- `Failed`：输入前置检查失败、XSD 无效或依赖缺失、Schema 加载失败、Xerces 初始化失败或内部异常等阻断性问题；通常不填充 XML 错误表格，而在 `message` 中给出可理解原因。
- 仅能定位到 XML 内容行列号的问题才构造 `ValidationError`。XSD 自身错误、文件问题和运行时异常必须通过 `Failed` 与日志处理。

## 5. 校验流程与 Xerces 使用要求

实现逻辑应遵守以下顺序：

1. GUI 检查 XML/XSD 是否已选择，并调用输入检查模块。
2. 输入检查模块验证文件状态，并输出绝对且词法规范化的路径。
3. `XmlValidator` 初始化并配置 Xerces-C++，启用命名空间和 XML Schema 校验。
4. 先加载并确认 XSD 可用；XSD 语法、`include`/`import` 或依赖路径错误属于 `Failed`。
5. 仅当 XSD 成功加载后，解析 XML 并执行格式良好性和 XSD 校验。
6. 自定义错误处理器收集 XML 解析与校验错误，转换为 `ValidationError`。
7. 返回完整的 `ValidationResult`，并记录校验摘要及运行异常。

- 需要妥善管理 Xerces 全局初始化与释放，避免重复初始化、资源泄漏和异常穿透 GUI 事件循环。
- 解析器配置、Schema 处理和 Xerces 字符串转换必须封装在非 GUI 层。
- 第一版不提供多 Schema 映射、用户手动配置命名空间/`schemaLocation`、远程文件或网络服务校验。

## 6. GUI 行为约束

- 主窗口必须提供 XML 文件选择、XSD 文件选择、开始校验和结果区域。
- XML 与 XSD 均选择后才允许执行校验；校验期间禁用重复提交。
- 初始状态不显示结果提示和错误表格。
- `Valid` 时显示绿色“XML 通过校验”提示，不显示错误表格。
- `Invalid` 时显示红色“XML 未通过校验”提示、错误数量和可滚动错误表格。表格列固定为：级别、行号、列号、错误描述。
- `Failed` 和输入问题使用明确的阻断性提示/弹窗；普通 XML 校验不通过不得使用弹窗打断用户。
- 关闭文件对话框时保持当前状态，不将其视为错误。

## 7. 异常、日志与性能

- 对文件不存在、目录路径、无读取权限、空文件、无效 XSD、XSD 依赖缺失、Xerces 异常及未预期异常提供清晰且可操作的提示。
- 文件扩展名不符合预期可警告，但不应仅因扩展名而错误拒绝可读取的文件。
- 顶层必须避免未处理异常导致程序崩溃；运行时异常写入系统日志。
- XML 校验错误记录行号、列号、级别和描述；运行日志可记录路径、开始/结束时间、状态和异常摘要。
- 错误数量很大时，界面需要避免无界渲染导致卡顿；完整信息优先保留在结果/日志中。
- 第一版以正确性为先，不为超大 XML 提前引入复杂并发、取消或批处理机制。

## 8. 测试与验证

- 核心校验逻辑使用 GTest；GUI 行为尽量通过可测试的展示/状态逻辑与业务逻辑解耦。
- 优先复用 `tests/xml/valid/`、`tests/xml/invalid/` 与 `tests/xsd/` 中的样例。新增回归用例时按合法/非法场景分类存放。
- 至少覆盖：合法 XML、格式不良好 XML、XSD 结构不匹配、必填元素/属性缺失、类型或枚举值错误、输入路径异常、无效 XSD 与 XSD 依赖加载失败。
- 修改 CMake 或核心校验代码后，优先运行受影响的 GTest；在依赖齐备时再运行 CMake 配置、构建和完整测试。
- 不要通过弱化 XSD 约束、吞掉异常或删除失败样例来让测试“通过”。

## 9. 代码与变更约定

- 使用清晰的英文类、函数、变量和文件名；面向用户的界面文本、日志说明与开发文档使用中文。
- 具体编码规则参见 `docs/项目编码规范.md`，包括头文件宏定义保护和 C++17 特性使用约定。
- 保持单一职责和最小改动原则，避免无关重构或提前实现本版本范围外的功能。
- 对跨模块公共类型使用稳定、显式的头文件；避免 GUI、校验器和 Xerces 封装之间产生循环依赖。
- Git 提交信息遵循 Conventional Commits 规范：`type(scope): subject`。
- 新功能、接口语义或范围发生变化时，先更新 `docs/概要设计报告.md`，并同步更新测试和本文件中受影响的约定。
