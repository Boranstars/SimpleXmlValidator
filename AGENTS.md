# SimpleXmlValidator 维护指南

## 1. 文档与版本状态

- 项目第一版已完成，当前工作以缺陷修复、回归验证、文档同步和受控功能演进为主；避免重新执行已完成阶段的实现分工。
- `docs/概要设计报告.md` 是功能语义、对外接口和版本范围的最高依据。改变这些内容时，先更新该文档，再更新实现、测试和本文件。
- `docs/模块实施计划.md` 记录已完成阶段、验收记录和后续里程碑；仅在范围、里程碑或验收结论变化时更新，不将日常小修复逐项写入。
- `README.md` 是构建、发布和使用说明；修改构建预设、依赖、交付物或用户可见行为时同步更新。
- `docs/项目编码规范.md` 规定头文件保护和 C++17 使用约定。更深层目录若有 `AGENTS.md`，其规则优先。

## 2. 当前范围与行为契约

- 工具校验本地单个 XML 文件和单个 XSD 文件；支持 XSD 的本地 `include`/`import`，拒绝网络资源。
- 路径使用 `std::filesystem::path`。输入检查必须验证空路径、存在性、普通文件、可读性和空文件，并返回绝对、词法规范化路径；必须支持中文和空格路径。
- `ValidationResult` 是 GUI、日志和测试之间的稳定结果模型：
  - `Valid`：XML 格式良好且通过已加载的 XSD。
  - `Invalid`：校验正常完成，但 XML 格式或内容不符合约束；仅 XML 内容诊断写入 `errors`。
  - `Failed`：输入、XSD、XSD 依赖、Xerces 初始化或内部异常阻断流程；使用 `message` 说明原因，通常不填充 XML 错误表格。
- 不得将 XSD 自身错误、文件错误或运行时异常伪装为带 XML 行列号的 `ValidationError`。
- 第一版不支持批量校验、网络或数据库数据源、手工 Schema 映射、XML/XSD 编辑、报告导出或业务语义校验。

## 3. 架构与依赖边界

```text
src/core/validation/          结果模型、输入检查、XmlValidator 编排
src/infrastructure/xerces/    Xerces 运行时、字符串、错误收集、资源解析、Schema 校验
src/infrastructure/logging/   日志器与校验记录
src/gui/                      Qt 主窗口、展示映射、只读 XML 视图与样式
```

- `SimpleXmlValidatorCore` 承载 core 与 infrastructure，实现目标私有链接 Xerces-C++ 和 spdlog。
- `SimpleXmlValidator` 仅链接 Qt 与 `SimpleXmlValidatorCore`；GUI 不得包含 Xerces 或 spdlog 头文件，也不得解析原始异常对象。
- `XmlValidator` 通过构造函数接收已初始化的 `XercesRuntime`，可选接收 `LogManager`；运行时由 `main.cpp` 作为组合根持有，生命周期必须覆盖校验器和解析对象。
- Xerces 原始类型不得泄漏到 `core/validation` 的公共结果模型或 GUI；第三方异常必须在 infrastructure 层转换。
- 日志失败只能静默降级或记录到可用通道，绝不能改变校验结果或中断主流程。
- 已知技术债：`XercesString` 仍依赖系统本地代码页转码；Windows 非 UTF-8 代码页下，包含中文 XML 名称的 Xerces 诊断可能乱码。涉及字符串转换时优先解决该问题，并补充 Windows 回归验证。

## 4. GUI 与展示规则

- XML 和 XSD 均选择后才能校验；校验期间禁止重复提交；取消文件对话框不得清空已有选择。
- `ValidationResultPresenter` 负责将校验结果映射为 GUI 状态，并保持可脱离 Qt 的 GTest 覆盖。
- `Valid` 显示通过状态，不显示错误表格；`Invalid` 在主界面展示错误；`Failed` 使用明确的阻断性提示，不把普通 XML 不合格当作弹窗错误。
- 错误表格固定为“级别、文件、行号、列号、错误描述”五列，支持分页、详情和只读上下文高亮；展示层可合并重复诊断，但日志和结果模型保留原始明细。
- 保持 `ValidationResultPresenter::kMaxErrorRows` 的防御性上限，禁止引入无界表格渲染。

## 5. 构建与测试

- 使用 C++17、CMake 3.30+、Qt 5.15.x、vcpkg manifest（Xerces-C++、spdlog、GTest）。不要硬编码第三方或 Qt 安装路径。
- 优先使用 `CMakePresets.json`：Linux 配置 `cmake --preset linux-vcpkg -DCMAKE_BUILD_TYPE=Release`，构建 `cmake --build --preset linux-vcpkg-release`，测试 `xvfb-run --auto-servernum ctest --preset linux-vcpkg-release`。
- 修改核心校验、Xerces 封装、日志或 CMake 后，先运行受影响的 GTest，再运行对应平台的完整 CTest；修改 GUI 展示映射时至少运行 Presenter GTest 和离屏 GUI 冒烟验证。
- 复用 `tests/xml/valid/`、`tests/xml/invalid/` 和 `tests/xsd/`；修复缺陷必须补充最小回归测试或 fixture。不得通过放松 XSD、吞掉异常或删除失败样例让测试通过。
- 新增源文件、资源、测试或依赖时，同步更新相应 `CMakeLists.txt`、测试说明和必要的构建文档。

## 6. 变更纪律

- 使用清晰的英文标识符；用户界面文本、日志说明和开发文档使用中文。
- 维持单一职责和最小改动，避免无关重构。公共类型放在稳定、显式的头文件中，避免循环依赖。
- 接口语义、支持范围或用户可见 GUI 行为变更时：更新概要设计、相关 GTest/fixture、README 和实施计划；仅内部重构无需修改概要设计。
- Git 提交信息使用 Conventional Commits：`type(scope): subject`。
