# SimpleXmlValidator

[![CI](https://github.com/Boranstars/SimpleXmlValidator/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/Boranstars/SimpleXmlValidator/actions/workflows/ci.yml)
[![Latest Release](https://img.shields.io/github/v/release/Boranstars/SimpleXmlValidator)](https://github.com/Boranstars/SimpleXmlValidator/releases)
[![License](https://img.shields.io/github/license/Boranstars/SimpleXmlValidator)](LICENSE)

中文 | [English](README.en.md)

一个面向开发、测试和配置维护人员的桌面 XML 校验工具。选择本地 XML 文件和 XSD 文件后，工具会检查 XML 格式良好性并执行 XSD Schema 校验，将可定位的 XML 错误以级别、文件、行号、列号和描述的形式展示出来。

![SimpleXmlValidator 校验界面](docs/docpics/ui-overview.png)

## 功能

- 校验单个本地 XML 文件的格式良好性和 XSD Schema 约束。
- 区分“校验未通过”与“无法完成校验”：前者展示 XML 错误明细，后者给出可操作的阻断性提示。
- 支持中文路径、含空格路径，以及 XSD 的本地 `include` / `import` 依赖解析。
- 提供 XML 预览、语法高亮、错误详情和出错行附近的上下文展示。
- 对重复错误进行合并计数，并通过分页避免大量错误导致界面无界渲染。
- 记录系统运行日志和 XML 校验日志；日志不可用不会改变校验结果。

## 当前范围

当前版本聚焦于**本地、单个 XML 文件与单个 XSD 文件**的校验。暂不支持网络文件、批量校验、多个 Schema 映射、手动配置命名空间 / `schemaLocation`、业务语义校验或校验报告导出。

## 安装与运行

### 从 Release 安装

请在 [Releases](https://github.com/Boranstars/SimpleXmlValidator/releases) 页面下载与系统对应的附件。当前仓库最新标签为 [`v0.1.0`](https://github.com/Boranstars/SimpleXmlValidator/releases/tag/v0.1.0)（2026-07-23）；每个版本的发布流程会生成以下文件：

| 平台 | 发布文件 | 使用方式 |
| --- | --- | --- |
| Windows x64 | `SimpleXmlValidator-windows.zip` | 解压后运行 `SimpleXmlValidator.exe`。请保持解压目录内的 DLL 与 `plugins` 文件夹完整。 |
| Linux x86_64 | `SimpleXmlValidator-linux.AppImage` | 赋予执行权限后直接运行：`chmod +x SimpleXmlValidator-linux.AppImage && ./SimpleXmlValidator-linux.AppImage`。 |
| macOS | `SimpleXmlValidator-macos.dmg` | 打开 DMG，将应用拖入“应用程序”目录后启动。 |

发布附件同时包含 `SHA256SUMS.txt`，建议下载后用平台工具校验文件哈希。macOS 或 Windows 若提示未知开发者，请先核对下载来源和 SHA-256，再按系统安全策略允许启动。

### 基本使用

1. 启动程序，点击“打开 XML”选择待校验 XML 文件。
2. 点击“打开 XSD”选择对应的 XSD 文件。
3. 点击“开始校验”。两个文件均已选择时该操作才可用。
4. 查看结果：
   - **校验通过**：显示通过状态，不展示错误表格。
   - **校验未通过**：在“校验结果”中查看错误列表；选中一行可查看完整描述和 XML 上下文。
   - **校验失败**：例如文件不可读、XSD 无效或其依赖无法加载时，程序会显示阻断性原因。

程序默认在工作目录的 `logs/` 下写入运行和校验日志。

## 从源码构建

### 前置条件

- CMake **3.30** 或更高版本。
- 支持 C++17 的编译器：Windows 推荐 Visual Studio 2022，Linux 推荐 GCC 或 Clang，macOS 使用 Xcode Command Line Tools。
- [vcpkg](https://github.com/microsoft/vcpkg)，用于安装 `xerces-c`、`spdlog` 和 `gtest`。
- Qt **5.15.x**（CI 使用 Qt 5.15.2），包含 `Core`、`Gui` 和 `Widgets` 模块。
- Linux 图形环境依赖：Ubuntu/Debian 可安装 `libgl1-mesa-dev`；运行 GUI 测试时还需要 `xvfb`。

项目使用 manifest 模式管理 vcpkg 依赖，CMake 配置时会根据 `vcpkg.json` 自动安装所需库。克隆仓库后，设置以下环境变量：

```bash
export VCPKG_ROOT=/path/to/vcpkg
export QT_ROOT=/path/to/Qt/5.15.2/<kit>
```

其中 `<kit>` 在 Linux 通常为 `gcc_64`，Windows 通常为 `msvc2019_64` 或兼容套件；macOS 可设为 Homebrew Qt 5 前缀，例如 `$(brew --prefix qt@5)`。

### Linux x86_64

```bash
git clone https://github.com/Boranstars/SimpleXmlValidator.git
cd SimpleXmlValidator

export VCPKG_ROOT=/path/to/vcpkg
export QT_ROOT=/path/to/Qt/5.15.2/gcc_64

cmake --preset linux-vcpkg -DCMAKE_BUILD_TYPE=Release
cmake --build --preset linux-vcpkg-release
xvfb-run --auto-servernum ctest --preset linux-vcpkg-release
./out/build/linux-vcpkg/SimpleXmlValidator
```

### Windows x64（PowerShell）

```powershell
git clone https://github.com/Boranstars/SimpleXmlValidator.git
Set-Location SimpleXmlValidator

$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
$env:QT_ROOT = 'C:\Qt\5.15.2\msvc2019_64'

cmake --preset windows-vcpkg -DCMAKE_BUILD_TYPE=Release
cmake --build --preset windows-vcpkg-release
ctest --preset windows-vcpkg-release
.\out\build\windows-vcpkg\Release\SimpleXmlValidator.exe
```

### macOS

```bash
git clone https://github.com/Boranstars/SimpleXmlValidator.git
cd SimpleXmlValidator

brew install qt@5
export VCPKG_ROOT=/path/to/vcpkg
export QT_ROOT="$(brew --prefix qt@5)"

cmake --preset macos-vcpkg -DCMAKE_BUILD_TYPE=Release
cmake --build --preset macos-vcpkg-release
ctest --preset macos-vcpkg-release
open ./out/build/macos-vcpkg/SimpleXmlValidator.app
```

可选的开发检查：在 GCC 或 Clang 环境中，可在配置命令末尾加入 `-DSIMPLE_XML_VALIDATOR_ENABLE_ASAN=ON` 和 / 或 `-DSIMPLE_XML_VALIDATOR_ENABLE_UBSAN=ON`。

## 开发与贡献

欢迎通过 Issue 和 Pull Request 参与改进。提交前请阅读 [`AGENTS.md`](AGENTS.md)、[`docs/概要设计报告.md`](docs/概要设计报告.md) 和 [`docs/项目编码规范.md`](docs/项目编码规范.md)；概要设计报告是实现语义的最高依据。提交 PR 前至少运行受影响的测试，依赖齐备时运行对应平台的 `ctest --preset <平台>-vcpkg-release`。提交信息使用 Conventional Commits 格式，例如 `feat(validation): 支持新的错误展示`。

CI 会在 Linux、Windows 和 macOS 上构建并运行测试；推送形如 `vX.Y.Z` 的标签会触发跨平台打包和 GitHub Release 发布。

## 技术栈

- C++17、CMake、Qt 5.15.x（Core / Gui / Widgets）
- Xerces-C++（XML 解析与 XSD 校验）
- spdlog（日志）
- GoogleTest（自动化测试）
- GitHub Actions（跨平台 CI/CD）

## 许可证

本项目采用 [MIT License](LICENSE)。
