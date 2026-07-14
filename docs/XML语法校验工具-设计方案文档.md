# XML语法校验工具 设计方案文档

## 1. 项目背景

在软件开发过程中，XML（可扩展标记语言）被广泛应用于数据交换、配置管理和系统间通信等场景。一个格式正确、结构规范的XML文档是保障数据准确性和系统稳定性的前提。

本次任务要求实习生使用C++实现一个XML语法校验工具，用于检查XML文件的格式正确性，帮助开发团队快速定位XML文档中的语法问题。

## 2. 项目目标

- 开发一个C++命令行或带界面的工具，接收XML文件和对应的XSD文件作为输入
- 界面用Qt框架QT5.12.x
- 校验XML文档是否符合W3C XML语法规范
- 支持XML Schema（XSD）验证
- 输出清晰的校验结果，包括通过/失败状态及详细的错误信息
- 具备良好的跨平台兼容性（Windows/Linux）

## 3. XML语法校验规则

### 3.1 格式良好性检查

一个“形式良好”（Well-Formed）的XML文档必须遵守以下基本语法规则：

| 序号 | 规则       | 说明                       |
| -- | -------- | ------------------------ |
| 1  | 必须有根元素   | XML文档有且只有一个根元素           |
| 2  | 标签必须闭合   | 所有元素都必须有对应的关闭标签          |
| 3  | 标签大小写敏感  | `<Book>`和`<book>`被视为不同元素 |
| 4  | 元素必须正确嵌套 | 不能交叉嵌套，如`<a><b></a></b>` |
| 5  | 属性值必须加引号 | 所有属性值必须用双引号或单引号括起来       |
| 6  | 特殊字符需转义  | `&`、`<`、`>`等需使用实体引用      |

本项目不仅需要检查上述“形式良好”性，还需要进一步支持通过XML Schema（XSD）进行结构验证。

### 3.2 Schema结构验证

为什么要做XSD Schema验证?

\
“形式良好”（Well-Formed）只保证XML语法正确，但不保证数据内容有效。例如：

订单金额字段误填了负数或文本

日期字段格式不符合要求（如2026/07/01而非2026-07-01）

必填字段缺失或出现未定义的额外元素

XSD Schema验证能够精准捕获上述语义级错误，确保XML文档不仅“写得对”，而且“内容对”。

## 4. 技术选型

### 4.1 可选C++ XML解析库对比

| 库名称        | 特点                                          | 适用场景                            |
| ---------- | ------------------------------------------- | ------------------------------- |
| Xerces-C++ | 高性能、模块化，支持DOM、SAX和SAX2 API，完整支持XML Schema验证 | 处理大量XML数据、需要高性能解析和完整Schema验证的项目 |
| libxml2    | 功能强大，跨平台，API丰富                              | 需要处理复杂XML数据的跨平台桌面应用             |
| MSXML      | Microsoft官方库，与Windows平台集成度高                 | Windows平台下的VC++开发               |
| pugixml    | 高性能、轻量级，支持UTF-8编码                           | 游戏开发或其他高性能要求的应用                 |
| TinyXML2   | 体积小、依赖少、简单易用                                | 嵌入式、小型工具开发                      |

### 4.2 推荐方案

推荐使用 Xerces-C++

，理由如下：因为需要支持XML Schema（XSD）验证，而Xerces-C++完整支持XML Schema 1.0和大部分1.1特性。

- 完全支持XML Schema 1.0和大部分1.1特性
- Apache开源项目，文档完善，社区活跃
- 跨平台支持良好（Windows/Linux/macOS）
- 同时支持DOM和SAX两种解析方式，灵活性高
- 在C++ XML解析库社区推荐度排名靠前

## 5. 设计方案

### 5.1 系统架构

```
┌─────────────────────────────────────────────────────────┐
│              命令行入口 (main)/或者界面入口               │
│                                                         │
└─────────────────────┬───────────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────────┐
│                   XML校验引擎                           │
│  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │  格式良好性检查   │  │   Schema结构验证             │  │
│  │  (Well-Formed)   │  │  (XSD Validation)          │  │
│  └─────────────────┘  └─────────────────────────────┘  │
└─────────────────────┬───────────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────────┐
│                   错误处理器                            │
│           收集、格式化并输出校验错误信息                  │
└─────────────────────────────────────────────────────────┘
```

### 5.2 核心模块设计

（1）XSD文件编写

- 根据业务表格编写XSD Schema文件
- 支持自定义元素、属性、约束等XML Schema特性

  示例：假设公司的业务表格定义了如下XML格式（示例）：

| 元素/属性               | 类型   | 出现次数  | 说明                |
| ------------------- | ---- | ----- | ----------------- |
| order（根元素）          | 复合类型 | 1次    | 订单根节点             |
| order/@id           | 字符串  | 必填    | 订单编号，格式：ORD-数字    |
| order/@date         | 日期   | 必填    | 下单日期，格式YYYY-MM-DD |
| customer/name       | 字符串  | 必填    | 客户姓名，长度≤50        |
| customer/email      | 字符串  | 可选    | 客户邮箱              |
| items/item          | 复合类型 | 1次或多次 | 商品明细              |
| items/item/@seq     | 整数   | 必填    | 商品序号              |
| items/item/name     | 字符串  | 必填    | 商品名称              |
| items/item/price    | 小数   | 必填    | 单价，≥0             |
| items/item/quantity | 正整数  | 必填    | 数量，≥1             |
| total               | 小数   | 必填    | 订单总金额             |

对应的XSD文件（order.xsd）

```xml
<?xml version="1.0" encoding="UTF-8"?>
<xs:schema 
    xmlns:xs="http://www.w3.org/2001/XMLSchema"
    targetNamespace="http://www.company.com/order"
    xmlns="http://www.company.com/order"
    elementFormDefault="qualified">

    <!-- ========== 根元素：order ========== -->
    <xs:element name="order">
        <xs:complexType>
            <xs:sequence>
                <!-- 客户信息 -->
                <xs:element name="customer" minOccurs="1" maxOccurs="1">
                    <xs:complexType>
                        <xs:sequence>
                            <xs:element name="name" type="nameType" />
                            <xs:element name="email" type="xs:string" minOccurs="0" />
                        </xs:sequence>
                    </xs:complexType>
                </xs:element>

                <!-- 商品明细列表 -->
                <xs:element name="items" minOccurs="1" maxOccurs="1">
                    <xs:complexType>
                        <xs:sequence>
                            <xs:element name="item" type="itemType" 
                                        minOccurs="1" maxOccurs="unbounded" />
                        </xs:sequence>
                    </xs:complexType>
                </xs:element>

                <!-- 订单总额 -->
                <xs:element name="total" type="nonNegativePriceType" />
            </xs:sequence>

            <!-- order元素的属性 -->
            <xs:attribute name="id" type="orderIdType" use="required" />
            <xs:attribute name="date" type="xs:date" use="required" />
        </xs:complexType>
    </xs:element>

    <!-- ========== 自定义类型定义 ========== -->

    <!-- 订单编号：格式 ORD-数字 -->
    <xs:simpleType name="orderIdType">
        <xs:restriction base="xs:string">
            <xs:pattern value="ORD-\d+" />
        </xs:restriction>
    </xs:simpleType>

    <!-- 客户姓名：长度不超过50 -->
    <xs:simpleType name="nameType">
        <xs:restriction base="xs:string">
            <xs:maxLength value="50" />
        </xs:restriction>
    </xs:simpleType>

    <!-- 商品条目类型 -->
    <xs:complexType name="itemType">
        <xs:sequence>
            <xs:element name="name" type="xs:string" />
            <xs:element name="price" type="nonNegativePriceType" />
            <xs:element name="quantity" type="positiveIntegerType" />
        </xs:sequence>
        <xs:attribute name="seq" type="xs:positiveInteger" use="required" />
    </xs:complexType>

    <!-- 非负金额（≥0） -->
    <xs:simpleType name="nonNegativePriceType">
        <xs:restriction base="xs:decimal">
            <xs:minInclusive value="0" />
            <xs:fractionDigits value="2" />
        </xs:restriction>
    </xs:simpleType>

    <!-- 正整数（≥1） -->
    <xs:simpleType name="positiveIntegerType">
        <xs:restriction base="xs:integer">
            <xs:minInclusive value="1" />
        </xs:restriction>
    </xs:simpleType>

</xs:schema>
```

XSD编写要点

| 要点                             | 说明                                                              |
| ------------------------------ | --------------------------------------------------------------- |
| 根元素schema                      | 必须包含XML Schema命名空间xmlns:xs="<http://www.w3.org/2001/XMLSchema>" |
| targetNamespace                | 定义该XSD所属的目标命名空间，供XML文档引用                                        |
| elementFormDefault="qualified" | 要求XML中的元素必须带命名空间前缀                                              |
| 复合类型complexType                | 包含子元素或属性的元素必须声明为复合类型                                            |
| 出现次数控制                         | 使用minOccurs和maxOccurs控制元素出现次数，unbounded表示无限次                    |
| 数据类型约束                         | 使用xs:string、xs:date、xs:decimal等内置类型                             |
| 值域限定                           | 使用restriction+minInclusive、maxLength、pattern等限定可接受值             |

（2）命令行参数解析模块(命令行程序)

- 支持输入XML文件路径（必需）
- 支持XSD Schema文件路径（用于结构验证）
- 支持详细错误输出

（3）XML解析与校验模块

- 使用Xerces-C++的`XercesDOMParser`创建解析器实例
- 启用校验模式：`parser.setValidationScheme(XercesDOMParser::Val_Always)`
- 启用Schema支持：`parser.setDoSchema(true)`

（4）错误处理模块

- 实现自定义`ErrorHandler`继承自`HandlerBase`
- 捕获并记录所有校验错误（包括警告和致命错误）
- 输出格式化的错误报告（文件名、行号、列号、错误描述）

（5）结果输出模块

- 校验通过：输出 `✓ Validation passed: [文件名]`
- 校验失败：输出 `✗ Validation failed: [文件名]` 及详细错误列表

### 5.3 核心代码示例（基于Xerces-C++）

```cpp
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <iostream>

XERCES_CPP_NAMESPACE_USE

class MyErrorHandler : public HandlerBase {
public:
    void warning(const SAXParseException& exc) override {
        // 处理警告
    }
    void error(const SAXParseException& exc) override {
        // 处理错误
        std::cerr << "Error at line " << exc.getLineNumber() 
                  << ", column " << exc.getColumnNumber() 
                  << ": " << XMLString::transcode(exc.getMessage()) << std::endl;
    }
    void fatalError(const SAXParseException& exc) override {
        // 处理致命错误
    }
};

int main(int argc, char* argv[]) {
    // 初始化Xerces-C++环境
    XMLPlatformUtils::Initialize();
    
    // 创建解析器
    XercesDOMParser parser;
    parser.setValidationScheme(XercesDOMParser::Val_Always);
    parser.setDoSchema(true);
    parser.setDoNamespaces(true);
    
    // 设置错误处理器
    MyErrorHandler errHandler;
    parser.setErrorHandler(&errHandler);
    
    try {
        // 解析并校验XML文件
        parser.parse("example.xml");
        if (parser.getErrorCount() == 0) {
            std::cout << "XML document is valid." << std::endl;
        } else {
            std::cout << "XML document is invalid. Errors: " 
                      << parser.getErrorCount() << std::endl;
        }
    } catch (const XMLException& e) {
        std::cerr << "Parse error: " 
                  << XMLString::transcode(e.getMessage()) << std::endl;
    }
    
    // 清理资源
    XMLPlatformUtils::Terminate();
    return 0;
}
```

## 6. 项目环境配置

### 6.1 Windows（Visual Studio）

1. 下载Xerces-C++预编译库（或从源码编译）
2. 在项目属性中配置包含目录和库目录
3. 链接必要的库文件（`xerces-c_3.lib`）
4. 确保运行时DLL（`xerces-c_3_2.dll`）在可执行文件目录或系统PATH中

### 6.2 Linux（g++）

```bash
# 安装libxerces-c-dev
sudo apt-get install libxerces-c-dev

# 编译
g++ -o xml_validator main.cpp -lxerces-c
```

## 7. 错误处理与日志

### 7.1 错误分级

| 级别                | 说明            | 处理方式            |
| ----------------- | ------------- | --------------- |
| 警告（Warning）       | 不影响解析但建议关注的问题 | 记录日志，继续解析       |
| 错误（Error）         | 违反XML语法规则     | 记录详细位置信息，继续尝试解析 |
| 致命错误（Fatal Error） | 严重问题导致无法继续    | 停止解析，返回非零退出码    |

### 7.2 错误信息格式

```
[ERROR] example.xml: line 15, column 8: Element 'book' is not closed.
[ERROR] example.xml: line 22, column 12: Attribute 'id' value must be quoted.
```

## 8. 测试计划

### 8.1 测试用例设计

| 测试场景                   | 测试文件                    | 预期结果    |
| ---------------------- | ----------------------- | ------- |
| 格式正确的XML               | `valid.xml`             | 校验通过    |
| 缺少根元素                  | `no_root.xml`           | 报告错误    |
| 标签未闭合                  | `unclosed.xml`          | 报告错误及行号 |
| 标签大小写不一致               | `case_mismatch.xml`     | 报告错误    |
| 嵌套错误                   | `bad_nesting.xml`       | 报告错误    |
| 属性值缺少引号                | `no_quotes.xml`         | 报告错误    |
| 包含特殊字符未转义              | `special_char.xml`      | 报告错误    |
| 空文件                    | `empty.xml`             | 报告错误    |
| XSD校验正确的XML            | `xsd_valid.xml`         | 校验通过    |
| 数据类型错误                 | `data_type_error.xml`   | 报告错误    |
| 缺失必填字段                 | `missing_fields.xml`    | 报告错误    |
| 包含不存在的属性               | `missing_attribute.xml` | 报告错误    |
| 包含不存在的节点               | `missing_node.xml`      | 报告错误    |
| 节点个数不正确（例如定义只有一个却包含多个） | `node_count.xml`        | 报告错误    |

### 8.2 测试工具

- 可结合使用 `xmllint`（Linux）或在线XML校验器进行结果对比验证
- 建议编写自动化测试脚本，批量运行测试用例

## 9. 交付物清单

- 概要设计文档
- 完整的C++源代码（含CMakeLists.txt或Makefile）
- **对应的XSD Schema文件**（根据业务表格编写）
- 项目编译与运行说明文档（README）
- 若干测试用XML文件（**包含通过验证的正确示例和未通过验证的错误示例**）
- 可执行程序（或编译指南）
- 测试报告

## 10. 学习建议

对于刚接触XML校验的实习生，建议按以下路径学习：

1. 了解XML基础语法  ：先通读XML规范，理解“形式良好”和“有效”的区别
2. C++编程知识学习；
3. 熟悉所选库的API  ：阅读Xerces-C++官方文档和示例代码
4. 从简单功能开始  ：先实现基本的“形式良好”性检查，再扩展Schema验证
5. 善用调试工具  ：使用XML编辑器的语法高亮和校验功能辅助调试

## 11. 附录A（资料性）报告文件格式定义

### A.1 智能诊断报告文件定义

智能诊断报告文件采用XML格式存储，使用UTF-8格式字符编码。文件命名为“STAT\_功能码\_时间信息.xml”，包括以下内容：

a) 功能码用于区分功能类型，见表A.1。

b) 时间信息格式为：年（四位）月（两位）日（两位）\_时（两位）分（两位）秒（两位）\_毫秒（三位）。

                                                                                  **表A.1  功能码定义**

| 功能码 | 功能描述     | 关键字            |
| :-- | :------- | :------------- |
| 01  | 二次回路故障诊断 | SecCircuit     |
| 02  | 状态监测预警   | StatusMonitor  |
| 03  | 同源数据比对   | SamComp        |
| 04  | 定值和版本校核  | Setting        |
| 05  | 装置异常告警定位 | DevAlaLocation |

###### A.1.1 二次回路故障诊断报告

| 层次            | 元素/属性        | 说明                                      | 类型        | M/O | 个数 |
| :------------ | :----------- | :-------------------------------------- | :-------- | :-- | :- |
| 第0级           | STAT         | 根元素                                     | -         | M   | 1  |
| STAT属性1       | type         | 同功能码定义，用于区分功能类型                         | STRING    | M   | 1  |
| STAT属性2       | desc         | 功能码的中文描述                                | STRING    | M   | 1  |
| 第1级           | QueryTime    | 查询时间信息段                                 | -         | M   | 1  |
| QueryTime属性1  | start        | 起始时间a                                   | -         | M   | 1  |
| QueryTime属性2  | end          | 结束时间a                                   | -         | M   | 1  |
| 第2级           | Warning      | 告警信息段，查询区段内回路诊断告警信息                     | -         | M   | ≥1 |
| Warning属性1    | type         | SecCircuit                              | STRING    | M   | 1  |
| Warning属性2    | time         | 此次告警记录产生的时间a                            |           | M   | 1  |
| 第3级           | IED          | IED告警信息段                                | -         | M   | ≥1 |
| IED属性1        | name         | IED名称                                   | STRING    | M   | 1  |
| IED属性2        | desc         | IED描述                                   | STRING    | M   | 1  |
| 第4级           | Point        | 告警点信息项，记录告警点信息                          | -         | M   | ≥1 |
| Point属性1      | path         | 信号reference                             | STRING    | M   | 1  |
| Point属性2      | desc         | 信号描述                                    | STRING    | M   | 1  |
| Point属性3      | other        | 其他                                      | STRING    | M   | 1  |
| 第5级           | Value        | 信号状态值                                   | BOOL      | M   | 1  |
| 第5级           | Time         | 信号产生此变位时间a                              | Datetime  | M   | 1  |
| 第5级           | Quality      | 信号品质信息（13位的位串）,与装置信号点品质位保持一致            | BITSTRING | M   | 1  |
| 第3级           | Locations    | 故障定位信息段                                 | -         | M   | 1  |
| 第4级           | Ied          | 故障装置信息段                                 | -         | O   | ≥1 |
| Ied属性1        | iedName      | 故障装置名称                                  | STRING    | M   | 1  |
| Ied属性2        | iedDesc      | 故障装置描述                                  | STRING    | M   | 1  |
| Ied属性3        | resultId     | 诊断结果序号，通过序号与Effect和Suggestion对应         | STRING    | M   | 1  |
| Ied属性4        | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第4级           | Slot         | 故障板卡信息段                                 | -         | O   | ≥1 |
| Slot属性1       | id           | 故障板卡编号                                  | STRING    | M   | 1  |
| Slot属性2       | iedName      | 故障板卡所属装置名称                              | STRING    | M   | 1  |
| Slot属性3       | iedDesc      | 故障板卡所属装置描述                              | STRING    | M   | 1  |
| Slot属性4       | resultId     | 诊断结果序号，通过序号与Effect和Suggestion对应         | STRING    | M   | 1  |
| Slot属性5       | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第4级           | Port         | 故障端口信息段                                 | -         | O   | ≥1 |
| Port属性1       | id           | 端口编号,端口号描述应为“板卡号-端口号”，标识如1-A            | STRING    | M   | 1  |
| Port属性2       | iedName      | 故障端口所属装置名称                              | STRING    | M   | 1  |
| Port属性3       | iedDesc      | 故障端口所属装置描述                              | STRING    | M   | 1  |
| Port属性4       | plug         | 故障端口类型，标识如ST、SC、LC、FC、MTRJ、RJ45         | STRING    | M   | 1  |
| Port属性5       | resultId     | 诊断结果序号，通过序号与Effect和Suggestion对应         | STRING    | M   | 1  |
| Port属性6       | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第4级           | Cable        | 光纤信息段                                   | -         | O   | ≥1 |
| Cable属性1      | id           | 物理连接线编号                                 | STRING    | M   | 1  |
| Cable属性2      | startIedName | 起始装置名称                                  | STRING    | M   | 1  |
| Cable属性3      | startIedDesc | 起始装置描述                                  | STRING    | M   | 1  |
| Cable属性4      | endIedName   | 终止装置名称                                  | STRING    | M   | 1  |
| Cable属性5      | endIedDesc   | 终止装置名称                                  | STRING    | M   | 1  |
| Cable属性6      | cableLen     | 光纤长度                                    | STRING    | O   | 1  |
| Cable属性7      | resultId     | 诊断结果序号，通过序号与Effect和Suggestion对应         | STRING    | M   | 1  |
| Cable属性8      | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第3级           | Effects      | 故障影响信息段                                 | -         | O   | 1  |
| 第4级           | Effect       | 具体故障影响信息段                               | -         | M   | ≥1 |
| Effect属性1     | desc         | 故障影响描述                                  | STRING    | M   | 1  |
| Effect属性2     | resultId     | 诊断结果序号，通过序号与Location和Suggestion对应       | STRING    | M   | 1  |
| Effect属性3     | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第3级           | Suggestions  | 故障处理建议信息段                               | -         | O   | 1  |
| 第4级           | Suggestion   | 具体处理建议信息段                               | -         | M   | ≥1 |
| Suggestion属性1 | desc         | 处理建议描述                                  | STRING    | M   | 1  |
| Suggestion属性2 | resultId     | 诊断结果序号，通过序号与Location和Effect对应           | STRING    | M   | 1  |
| Suggestion属性3 | priority     | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |

###### A.1.2 状态监测预警报告

| 层次            | 元素/属性     | 说明                                                            | 类型        | M/O | 个数 |
| :------------ | :-------- | :------------------------------------------------------------ | :-------- | :-- | :- |
| 第0级           | STAT      | 根元素                                                           | -         | M   | 1  |
| STAT属性1       | type      | 同功能码定义，用于区分功能类型                                               | STRING    | M   | 1  |
| STAT属性2       | desc      | 功能码的中文描述                                                      | STRING    | M   | 1  |
| 第1级           | QueryTime | 查询时间信息段                                                       | -         | M   | 1  |
| QueryTime属性1  | start     | 起始时间a                                                         | -         | M   | 1  |
| QueryTime属性2  | end       | 结束时间a                                                         | -         | M   | 1  |
| 第2级           | IED       | IED信息段                                                        | -         | M   | ≥1 |
| IED属性1        | name      | IED name                                                      | STRING    | M   | 1  |
| IED属性2        | desc      | IED描述                                                         | STRING    | M   | 1  |
| 第3级           | Warning   | 告警信息段，查询区段内此IED的告警记录                                          | -         | M   | ≥1 |
| Warning属性1    | type      | StatusMonitor                                                 | STRING    | M   | 1  |
| Warning属性2    | time      | 此次告警记录产生的时间a                                                  | -         | M   | 1  |
| Warning属性3    | subType   | 1 越限预警、2 同期比对、3 变化趋势、4 突变预警                                   |           | M   | 1  |
| 第4级           | Point     | 告警点信息项，记录告警点信息                                                | -         | M   | ≥1 |
| Point属性1      | path      | Reference                                                     | STRING    | M   | 1  |
| Point属性2      | desc      | 告警点描述                                                         | STRING    | M   | 1  |
| Point属性3      | other     | 其他                                                            | STRING    | M   | 1  |
| 第5级           | Datagrp   | 数据组信息段                                                        | -         | M   | ≥1 |
| Datagrpnum属性1 | num       | 数据个数                                                          | INT32     | M   | 1  |
| 第6级           | Data      | 具体数据信息段                                                       | -         | M   | ≥1 |
| data属性1       | value     | 数据值                                                           | FLOAT     | M   | 1  |
| data属性2       | datetime  | 数据时间                                                          | STRING    | M   | 1  |
| data属性3       | quality   | 告警点品质信息#13位码                                                  | BITSTRING | M   | 1  |
| 第5级           | Highlimit | 上门槛值信息段                                                       | -         | M   | 1  |
| Highlimit属性1  | value     | 上门槛值                                                          | FLOAT     | M   |    |
| 第5级           | Lowlimit  | 下门槛值信息段,对于同期比对可以省略下门槛信息段                                      | -         |     | 1  |
| Lowlimit属性1   | value     | 下门槛值                                                          | FLOAT     |     |    |
| 第5级           | Alarm     | 模拟量告警使能，1表示此信号使能此告警业务                                         | BOOLEAN   | M   | 1  |
| 第4级           | Policy    | 诊断规则信息段                                                       | -         | M   | 1  |
| 第5级           | Condition | 诊断规则条目                                                        | -         | M   | ≥1 |
| Condition属性1  | desc      | 诊断规则条目描述                                                      | STRING    | M   | 1  |
| Condition值    | -         | 在“时间周期”内告警点动作次数大于“模拟量告警次数”则产生告警，“持续时间内”只报一次，时间单位为秒。持续时间描述可省略。 | INT32     | M   | 1  |
| 第4级           | Result    | 结论描述字符串，自定义                                                   | STRING    | M   | 1  |

###### A.1.3 同源数据比对诊断报告

| 层次           | 元素/属性     | 说明                      | 类型     | M/O | 个数 |
| :----------- | :-------- | :---------------------- | :----- | :-- | :- |
| 第0级          | STAT      | 根元素                     | -      | M   | 1  |
| STAT属性1      | type      | 同功能码定义，用于区分功能类型         | STRING | M   | 1  |
| STAT属性2      | desc      | 功能码的中文描述                | STRING | M   | 1  |
| 第1级          | QueryTime | 查询时间信息段                 | -      | M   | 1  |
| QueryTime属性1 | start     | 起始时间a                   | -      | M   | 1  |
| QueryTime属性2 | end       | 结束时间a                   | -      | M   | 1  |
| 第2级          | Warning   | 告警信息段                   |        | M   | ≥1 |
| Warning属性1   | type      | SamComp                 | STRING | M   | 1  |
| Warning属性2   | time      | 告警时间a                   | STRING | M   | 1  |
| 第3级          | Group     | 同源通道组信息段                | -      | M   | ≥1 |
| Group属性1     | type      | 通道类型（1：模拟量通道，2：开关量通道）   | STRING | M   | 1  |
| Group属性2     | time      | 采样时刻                    | STRING | M   | 1  |
| 第4级          | Channel   | 比对通道信息段                 | -      | M   | ≥1 |
| Channel属性1   | path      | Reference               | STRING | M   | 1  |
| Channel属性2   | desc      | 通道描述                    | STRING | M   | 1  |
| Channel属性3   | other     | 厂家自定义                   | STRING | M   | 1  |
| Channel属性4   | iedName   | 保护装置IedName或采集单元的LDname | STRING | M   | 1  |
| 第5级          | Data      | 通道数值信息段                 | -      | M   | 1  |
| Data属性1      | value     | 模拟量有效值或开关量状态            | STRING | M   | 1  |
| Data属性2      | unit      | 模拟量单位                   | STRING | O   | 1  |
| Data属性3      | ang       | 模拟量角度                   | FLOAT  | O   | 1  |
| 第4级          | Result    | 比对结果信息段                 | -      | M   | 1  |
| Result属性1    | desc      | 比对结果描述                  | STRING | M   | 1  |

###### A.1.4 保护版本校核报告

| 层次           | 元素/属性           | 说明                           | 类型     | M/O | 个数 |
| :----------- | :-------------- | :--------------------------- | :----- | :-- | :- |
| 第0级          | STAT            | 根元素                          | -      | M   | 1  |
| STAT属性1      | type            | 同功能码定义，用于区分功能类型              | STRING | M   | 1  |
| STAT属性2      | desc            | 功能码的中文描述                     | STRING | M   | 1  |
| 第1级          | QueryTime       | 查询时间信息段                      | -      | M   | 1  |
| QueryTime属性1 | start           | 起始时间a                        | -      | M   | 1  |
| QueryTime属性2 | end             | 结束时间a                        | -      | M   | 1  |
| 第2级          | IED             | IED信息段                       | -      | M   | ≥1 |
| IED属性1       | name            | IED name                     | STRING | M   | 1  |
| IED属性2       | desc            | IED描述                        | STRING | M   | 1  |
| 第3级          | Warning         | 告警信息段，查询区段内此IED的告警记录         | -      | M   | ≥1 |
| Warning属性1   | type            | Setting                      | STRING | M   | 1  |
| Warning属性2   | time            | 此次告警记录产生的时间a                 | -      | M   | 1  |
| Warning属性3   | subType         | 此次告警子类型，1表示定值比对告警，2表示版本检核告警。 | STRING | O   | 1  |
| 第4级          | Point           | 告警点信息项，记录告警点信息               | -      | M   | ≥1 |
| Point属性1     | path            | Reference                    | STRING | M   | 1  |
| Point属性2     | desc            | 告警点描述                        | STRING | M   | 1  |
| Point属性3     | other           | 其他                           | STRING | M   | 1  |
| 第5级          | Value           | 定值的值                         | FLOAT  | M   | 1  |
| 第5级          | Time            | 告警点产生此次告警的时间a                | -      | M   | 1  |
| 第4级          | CompareTemplate | 定值模板（上次召唤定值）                 | -      | M   | 1  |
| 第5级          | Point           | 模板定值信息项，记录模板定值信息             | -      | M   | ≥1 |
| Point属性1     | path            | Reference                    | STRING | M   | 1  |
| Point属性2     | desc            | 告警点描述                        | STRING | M   | 1  |
| Point属性3     | other           | 其他                           | STRING | M   | 1  |
| 第6级          | Value           | 定值的值                         | FLOAT  | M   | 1  |
| 第6级          | Time            | 模板生成的时间a                     | -      | M   | 1  |
| 第4级          | Result          | 结论描述字符串，自定义                  | STRING | M   | 1  |
| 第3级          | Warning         | 告警信息段，查询区段内此IED的告警记录         | -      | M   | ≥1 |
| Warning属性1   | type            | Version                      | STRING | M   | 1  |
| Warning属性2   | time            | 此次告警记录产生的时间a                 | -      | M   | 1  |
| Warning属性3   | subType         | 此次告警子类型，1表示定值比对告警，2表示版本检核告警。 | STRING | O   | 1  |
| 第4级          | Point           | 版本信息项，记录版本信息                 | -      | M   | ≥1 |
| Point属性1     | path            | Reference                    | STRING | M   | 1  |
| Point属性2     | desc            | 装置版本描述                       | STRING | M   | 1  |
| Point属性3     | other           | 其他                           | STRING | M   | 1  |
| 第5级          | Value           | 版本的值                         | STRING | M   | 1  |
| 第5级          | Time            | 版本产生此次告警的时间a                 | -      | M   | 1  |
| 第4级          | CompareTemplate | 版本模板（上次召唤版本）                 | -      | M   | 1  |
| 第5级          | Value           | 版本的值                         | STRING | M   | 1  |
| 第5级          | Time            | 上次版本的时间a                     | -      | M   | 1  |
| 第4级          | Result          | 结论描述字符串，自定义                  | STRING | M   | 1  |

###### A.1.5 装置异常告警定位报告

| 层次            | 元素/属性       | 说明                                      | 类型        | M/O | 个数 |
| :------------ | :---------- | :-------------------------------------- | :-------- | :-- | :- |
| 第0级           | STAT        | 根元素                                     | -         | M   | 1  |
| STAT属性1       | type        | 同功能码定义，用于区分功能类型                         | STRING    | M   | 1  |
| 层次            | 元素/属性       | 说明                                      | 类型        | M/O | 个数 |
| STAT属性2       | desc        | 功能码的中文描述                                | STRING    | M   | 1  |
| 第1级           | QueryTime   | 查询时间信息段                                 | -         | M   | 1  |
| QueryTime属性1  | start       | 起始时间a                                   | -         | M   | 1  |
| QueryTime属性2  | end         | 结束时间a                                   | -         | M   | 1  |
| 第2级           | Warning     | 告警信息段，查询区段内此IED的告警记录                    | -         | M   | ≥1 |
| Warning属性1    | type        | DevAlaLocation                          | STRING    | M   | 1  |
| Warning属性2    | time        | 此次告警记录产生的时间a                            | -         | M   | 1  |
| 第3级           | Locations   | 定位信息集                                   | -         | M   | 1  |
| 第4级           | Board       | 定位板卡信息段                                 | -         |     | ≥0 |
| Board属性1      | iedName     | 所属IED name                              | STRING    | M   | 1  |
| Board属性2      | iedDesc     | 所属IED中文描述                               | STRING    | M   | 1  |
| Board属性3      | slot        | 板卡号，如“1”                                | STRING    | M   | 1  |
| Board属性4      | desc        | 板卡描述                                    | STRING    | M   | 1  |
| Board属性5      | priority    | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 |           | M   | 1  |
| 第4级           | Location    | 定位信息段                                   | -         |     | ≥0 |
| Location属性1   | desc        | 定位结论，除板卡外的其他定位结论可以写在此处，内容自定义            |           | M   | 1  |
| Location属性2   | priority    | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 |           | M   | 1  |
| 第3级           | Effects     | 影响信息集                                   | -         | M   | 1  |
| 第4级           | SLoop       | 二次回路信息段                                 | -         |     | ≥0 |
| SLoop属性1      | desc        | 二次回路描述                                  | STRING    | M   | 1  |
| SLoop属性2      | priority    | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 | STRING    | M   | 1  |
| 第4级           | Effect      | 影响信息段                                   | -         |     | ≥0 |
| Effect属性1     | desc        | 影响信息描述，除回路外的其他影响信息可以写在此处，内容自定义          |           | M   | 1  |
| Effect属性2     | priority    | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 |           | M   | 1  |
| 第3级           | Suggestions | 处理建议信息集                                 | -         | M   | 1  |
| 第4级           | Suggestion  | 处理建议信息段                                 | STRING    | M   | ≥1 |
| Suggestion属性1 | desc        | 处理建议描述字符串，自定义                           |           | M   | 1  |
| Suggestion属性2 | priority    | 优先级，1表示优先级最高，每加1表示优先级降级，优先级高表示故障定位的准确度高 |           | M   | 1  |
| 第3级           | IED         | IED信息段                                  | -         | M   | ≥1 |
| IED属性1        | name        | IED name                                | STRING    | M   | 1  |
| IED属性2        | desc        | IED描述                                   | STRING    | M   | 1  |
| 第4级           | Point       | 告警点信息项，记录告警点信息                          | -         | M   | ≥1 |
| Point属性1      | path        | reference                               | STRING    | M   | 1  |
| Point属性2      | desc        | 告警点描述                                   | STRING    | M   | 1  |
| Point属性3      | other       | 其他                                      | STRING    | M   | 1  |
| 第5级           | Value       | 告警点值                                    | BOOL      | M   | 1  |
| 第5级           | Time        | 告警点产生此次告警的时间a                           | -         | M   | 1  |
| 第5级           | Quality     | 信号品质信息（13位的位串）,与装置信号点品质位保持一致            | BITSTRING | M   | 1  |
| 第5级           | AlaMeaning  | 告警含义                                    | -         | M   | 1  |

### A.2 定值智能比对报告

定值智能比对报告文件采用XML格式存储，使用UTF-8格式字符编码。文件命名为“SettingCheck\_AppName\_时间信息\_比对结果.xml”。

时间格式为：年（四位）月（两位）日（两位）\_时（两位）分（两位）秒（两位）\_毫秒（三位）。

比对结果为枚举值：包括“OK”和“ERR”

| 层次           | 元素/属性           | 说明                     | 类型     | M/O | 个数 |
| :----------- | :-------------- | :--------------------- | :----- | :-- | :- |
| 第0级          | APP             | 根元素                    | -      | M   | 1  |
| APP属性1       | appname         | 应用程序名                  | STRING | M   | 1  |
| APP属性2       | appdesc         | 应用名称                   | STRING | M   | 1  |
| 第1级          | QueryTime       | 校核时间信息段                | -      | M   | 1  |
| QueryTime属性1 | time            | 校核时间a                  | -      | M   | 1  |
| 第2级          | IED             | IED信息段                 | -      | M   | ≥1 |
| IED属性1       | name            | IED name               | STRING | M   | 1  |
| IED属性2       | desc            | IED描述                  | STRING | M   | 1  |
| 第3级          | Record          | 校核记录信息段，查询区段内此IED的校核记录 | -      | M   | 1  |
| Record属性1    | type            | Setting                | STRING | M   | 1  |
| Record属性2    | time            | 此次校核记录产生的时间a           | -      | M   | 1  |
| 第4级          | Point           | 定值点信息项，记录定值点信息         | -      | M   | ≥1 |
| Point属性1     | path            | Reference              | STRING | M   | 1  |
| Point属性2     | desc            | 定值点描述                  | STRING | M   | 1  |
| Point属性3     | other           | 其他                     | STRING | M   | 1  |
| 第5级          | Zone            |                        |        | M   | ≥1 |
| Zone属性1      | Number          | 校核定值所属区号               | INT    | M   | 1  |
| 第6级          | Value           | 定值的值                   | FLOAT  | M   | 1  |
| 第6级          | Time            | 定值值的获取时间a              | -      | M   | 1  |
| 第6级          | IsDifferent     | 与基准值比较是否一致。0：一致；1；不一致  | INT    | M   | 1  |
| 第3级          | CompareTemplate | 定值基准值                  | -      | M   | 1  |
| 第4级          | Point           | 定值信息项，记录定值基准值信息        | -      | M   | ≥1 |
| Point属性1     | path            | Reference              | STRING | M   | 1  |
| Point属性2     | desc            | 定值点描述                  | STRING | M   | 1  |
| Point属性3     | other           | 其他                     | STRING | M   | 1  |
| 第5级          | Zone            |                        |        | M   | ≥1 |
| Zone属性1      | Number          | 校核定值基准值所属区号            | INT    | M   | 1  |
| 第6级          | Value           | 定值的基准值                 | FLOAT  | M   | 1  |
| 第6级          | Time            | 该项基准值生成的时间a            | -      | M   | 1  |
| 第3级          | Result          | 结论描述字符串，自定义            | STRING | M   | 1  |

### A.3 二次安措防误报告

二次安措防误报告文件名格式定义为MntRpt\_年（XXXX）月（XX）日（XX）\_时（XX）分（XX）秒（XX）.xml，MntRpt\_20240425\_093035.xml，文件名时间与报告生成时间应保持一致。文件采用XML格式存储，使用UTF-8格式字符编码。

| 层次            | 元素/属性           | 说明                                                                                                                                                             | 类型     | M/O | 个数 |
| :------------ | :-------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------- | :----- | :-- | :- |
| 第0级           | MntRpt          | 根元素                                                                                                                                                            | -      | M   | 1  |
| 第1级           | Base            | 基本信息                                                                                                                                                           | -      | M   | 1  |
| Base属性1       | time            | 报告生成时间                                                                                                                                                         | STRING | M   | 1  |
| Base属性2       | type            | 填“在线监视”或“模拟预演”                                                                                                                                                 | STRING | M   | 1  |
| Base属性3       | objName         | 检修对象名称                                                                                                                                                         | STRING | M   | 1  |
| Base属性4       | total\_estimate | 本次检修安措的在线监视总体评价(1:问题检修; 0:正确检修)                                                                                                                                | INT    | M   | 1  |
| 第1级           | Substation      | 变电站信息                                                                                                                                                          | -      | M   | 1  |
| Substation属性1 | name            | 变电站名称，SCD模型变电站name                                                                                                                                             | STRING | M   | 1  |
| Substation属性2 | desc            | 变电站描述                                                                                                                                                          | STRING | M   | 1  |
| 第1级           | Bay             | 检修间隔信息                                                                                                                                                         | -      | O   | 1  |
| Bay属性1        | name            | 检修间隔名称，SCD模型间隔name                                                                                                                                             | STRING | O   | 1  |
| Bay属性2        | desc            | 检修间隔描述                                                                                                                                                         | STRING | O   | 1  |
| 第1级           | Ied             | 检修装置信息                                                                                                                                                         | -      | M   | ≥1 |
| Ied属性1        | name            | 检修装置名称，SCD模型装置iedname                                                                                                                                          | STRING | M   | 1  |
| Ied属性2        | desc            | 检修装置描述                                                                                                                                                         | STRING | M   | 1  |
| 第1级           | Step            | 安措状态信息                                                                                                                                                         |        | M   | ≥1 |
| Step属性1       | NO              | 安措序号                                                                                                                                                           | INT    | M   | 1  |
| Step属性2       | ied\_name       | 操作装置iedname                                                                                                                                                    | STRING | M   | 1  |
| Step属性3       | ied\_desc       | 操作装置描述desc                                                                                                                                                     | STRING | M   | 1  |
| Step属性4       | estimate        | 本条安措操作评价(1:问题检修项目; 0:正确检修项目)                                                                                                                                   | INT    | M   | 1  |
| Step属性5       | error\_ reason  | 评价的错误原因                                                                                                                                                        | STRING | M   | 1  |
| Step属性6       | type            | 安措操作类型，1：设备出口硬压板；2：相关运行保护功能软压板；3：相关运行线路保护断路器强制分位软压板；4：相关检修及运行保护GOOSE软压板；5：相关运行保护SV接收软压板；6：设备光纤或网线；7：设备检修硬压板；8：设备PT/CT二次回路连接片；9：闭锁装置硬压板或联线；10：相关运行母线保护隔离刀闸强制软压板 | INT    | M   | 1  |
| Step属性7       | object\_path    | 操作对像reference                                                                                                                                                  | STRING | M   | 1  |
| Step属性8       | value           | 0：退出 1：投入                                                                                                                                                      | INT    | M   | 1  |
| 第2级           | content         | 安措状态变化信息内容                                                                                                                                                     | STRING | M   | ≥1 |

### A.4 智能巡视报告

巡视报告文件采用XML格式存储，使用UTF-8格式编码，文件命名为“checkreport\_yyyyMMdd\_hhmmss.xml”，其中的“yyyyMMdd\_hhmmss”表示巡视的时间（年月日时分秒），年为4位数字，月、日、时、分、秒均为2位数字。

\<CheckReport>为xml文件的根节点，整个文件分为基本信息段（\<System>节点）、IED巡视概要结果段（\<Ied>节点）、巡视详细信息段（除\<System>节点、\<Ied>节点之外的其他节点）。

#### A.4.1 基本信息段

基本信息段（\<System>节点）描述了本次巡视的基本信息，包括厂站名、巡视时间、巡视原因、厂站保护设备数目、巡视的保护设备数目、发现异常的保护设备数目等。

| 字段名               | 含义        | 类型           | 说明                                       |
| :---------------- | :-------- | :----------- | :--------------------------------------- |
| Substation        | SCD中的变电站名 | string（1-64） | 变电站名                                     |
| CheckTime         | 巡视时间，精确到秒 | datetime     | 格式为：yyyy-MM-ddThh:mm:ss                  |
| CheckReason       | 巡视原因      | INT8         | 1: 自动巡视响应；2: 手工巡视响应                      |
| DeviceSum         | 接入装置数     | INT32        | 运维子站接入的装置总数                              |
| CheckDeviceSum    | 巡视装置数     | INT32        | 实际巡视的装置个数                                |
| AbnormalDeviceSum | 异常装置个数    | INT32        | 包含所有有问题的装置数，可能有问题的装置：1）通信中断装置2）巡视存在问题的装置 |

<br />

#### A.4.2 IED巡视概要结果段

IED巡视概要结果段（\<Ied>节点）以IED及其巡视项为单位，简要描述各IED的巡视情况，每个IED的巡视结果以一个\<Item>节点来表示。

| 字段名           | 含义                  | 类型            | 说明                                                   |
| :------------ | :------------------ | :------------ | :--------------------------------------------------- |
| DeviceId      | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                                          |
| DeviceName    | 装置名称                | string（1-64）  |                                                      |
| IsChecked     | 是否巡视                | BOOLEAN       | 0：未巡视；1：已巡视                                          |
| UnCheckReason | 未巡视或巡视失败原因          | INT16         | 0：不涉及（实际已巡视）；1：装置不在巡视范围内；2：装置通信中断；3：装置正在检修；4：未下发标准值。 |
| Result        | 巡视结果                | INT16         | 0：未知状态；1：正常不需检修；2：异常需关注；3：严重需检修；4：无法评价需人工确认。         |

\<Item>节点下包含了各巡视项巡视情况的子节点，其各节点含义如下：

| 节点名        | 含义        | 节点名         | 含义          |
| :--------- | :-------- | :---------- | :---------- |
| Zone       | 定值区巡视情况   | SoftVersion | 软件版本巡视情况    |
| Setting    | 定值巡视情况    | SelfAlarm   | 自检告警巡视情况    |
| SoftPlate  | 软压板巡视情况   | GpsAlarm    | GPS告警巡视情况   |
| HardPlate  | 硬压板巡视情况   | Wave        | 录波巡视情况      |
| Discrete   | 开入状态量巡视情况 | Analog      | 模拟量巡视情况     |
| Clock      | 时钟巡视情况    | Loop        | 电流、电压回路巡视情况 |
| CommStatus | 通信状态巡视情况  | Channel     | 光纤通道巡视情况    |
| SecCircuit | 二次回路巡视情况  | CRCCheck    | CRC巡视情况     |
| OpCheck    | 保护动作巡视情况  |             | <br />      |

各巡视项的巡视结果以节点的属性表示，其含义如下：

| 属性名           | 含义         | 类型      | 说明                                                  |
| :------------ | :--------- | :------ | :-------------------------------------------------- |
| IsChecked     | 是否巡视       | BOOLEAN | 0：未巡视；1：已巡视                                         |
| UnCheckReason | 未巡视或巡视失败原因 | INT16   | 0：不涉及（实际已巡视）；1：装置不在巡视范围内；2：装置通信中断；3：装置正在检修；4：未下发标准值 |
| result        | 巡视结果       | INT16   | 0：未知状态；1：正常不需检修；2：异常需关注；3：严重需检修；4：无法评价需人工确认。        |

<br />

#### A.4.3 巡视详细信息段

##### A.4.3.1 各节点命名与含义

巡视详细信息段包含了除\<System>节点、\<Ied>节点之外的其他节点，这些节点详细描述了各被巡视装置的各巡视项的巡视详细信息，各节点的含义如下：

| 节点名           | 含义        | 节点名         | 含义          |
| :------------ | :-------- | :---------- | :---------- |
| DiffZone      | 定值区巡视情况   | SoftVersion | 软件版本巡视情况    |
| DiffSetting   | 定值巡视情况    | SelfAlarm   | 自检告警巡视情况    |
| DiffSoftPlate | 软压板巡视情况   | GpsAlarm    | GPS告警巡视情况   |
| DiffHardPlate | 硬压板巡视情况   | Wave        | 录波巡视情况      |
| DiffDiscrete  | 开入状态量巡视情况 | DiffAnalog  | 模拟量巡视情况     |
| DiffClock     | 时钟巡视情况    | Loop        | 电流、电压回路巡视情况 |
| CommStatus    | 通信状态巡视情况  | Channel     | 光纤通道巡视情况    |
| SecCircuit    | 二次回路巡视情况  | CRCCheck    | CRC巡视情况     |
| OpCheck       | 保护动作巡视情况  | Sample      | 端口采样巡视情况    |
| DiffCurrent   | 差流巡视情况    |             | <br />      |

<br />

##### A.4.3.2 通信状态巡视详细信息段

通信状态巡视详细信息段（\<CommStatus>节点）描述站内所有已巡视装置的通信状态巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                              |
| :---------- | :------------------ | :------------ | :------------------------------ |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                     |
| RealVal     | 实际值                 | INT8          | 0：通信异常；1：通信正常                   |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻，格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                       |

<br />

##### A.4.3.3 **二次回路链路状态巡视详细信息段**

二次回路巡视详细信息段（\<SecCircuit>节点）描述站内所有已巡视装置的二次回路巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RealVal     | 实际值                 | INT8          |                                  |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.4 模拟量巡视详细信息段

模拟量信息段（\<DiffAnalog>节点，包括装置温度、工作电压）、端口采样信息段（\<Sample>节点，包括端口发送/接收光强）、差流信息段（\<DiffCurrent>，包括差动电流）分别描述了站内所有已巡视装置的装置温度及工作电压、端口采样、差流巡视情况；其格式同下面的\*\*“ 电流电压回路巡视”;\*\*

<br />

##### A.4.3.5 电流电压回路巡视详细信息段

模拟量、电流电压回路巡视详细信息段各字段命名与含义如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| ValType     | 数值类型                | string（1-64）  | 本信息段的数值类型为float（浮点数）             |
| RealVal     | 实际值                 | float         |                                  |
| LowerLimit  | 模拟量下限               | float         |                                  |
| UpperLimit  | 模拟量上限               | float         |                                  |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.6 开入状态量巡视详细信息段

开入状态量巡视详细信息段（\<DiffDiscrete>节点）描述了站内所有已巡视装置的开入状态量巡视情况、转换开关巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| ValType     | 数值类型                | string（1-64）  | 本信息段数值类型为int（整型）                 |
| RefVal      | 标准值                 | int           | 0：分；1：合                          |
| RealVal     | 实际值                 | int           | 0：分；1：合                          |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否有差异               | BOOLEAN       | 0：无差异；1：有差异                      |

<br />

##### A.4.3.7 硬压板、软压板巡视详细信息段

硬压板（\<DiffHardplate>节点）、软压板巡视详细信息段（\<DiffSoftplate>节点）分别描述了站内所有已巡视装置的硬、软压板巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| ValType     | 数值类型                | string（1-64）  | 本信息段数值类型为int（整型）                 |
| RefVal      | 标准值                 | int           | 0：退出；1：投入                        |
| RealVal     | 实际值                 | int           | 0：退出；1：投入                        |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否有差异               | BOOLEAN       | 0：无差异；1：有差异                      |

<br />

##### A.4.3.8 定值区巡视详细信息段

定值区巡视详细信息段（\<DiffZone>节点）描述了站内所有已巡视装置的定值区巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| ValType     | 数值类型                | string（1-64）  | 本信息段数值类型为int（整型）                 |
| RefVal      | 标准值                 | int           |                                  |
| RealVal     | 实际值                 | int           |                                  |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否有差异               | BOOLEAN       | 0：无差异；1：有差异                      |

<br />

##### A.4.3.9 定值巡视详细信息段

定值巡视详细信息段（\<DiffSetting>节点）描述了站内所有已巡视装置的定值巡视情况，其格式如下：

| 字段名       | 含义                  | 类型            | 说明                               |
| :-------- | :------------------ | :------------ | :------------------------------- |
| DeviceId  | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId   | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName | 信号点名称               | string（1-64）  |                                  |
| ValType   | 数值类型                | string（1-64）  | 本信息段数值类型为string（字符串）             |
| RefVal    | 标准值                 | string（1-64）  |                                  |
| RealVal   | 实际值                 | string（1-64）  |                                  |
| ZoneNo    | 定值项所属定值区            | int           |                                  |
| CheckTime | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |

<br />

##### A.4.3.10 软件版本巡视详细信息段

软件版本巡视详细信息段（\<SoftVersion>节点）描述了站内所有已巡视装置的软件版本巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RefVal      | 标准值                 | string（1-64）  |                                  |
| RealVal     | 实际值                 | string（1-64）  |                                  |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否有差异               | BOOLEAN       | 0：无差异；1：有差异                      |

<br />

##### A.4.3.11 通道巡视详细信息段

通道巡视详细信息段（\<Channel>节点）描述了站内所有已巡视装置的通道巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RealVal     | 实际值                 | INT8          | 0：未告警（复归）；1：告警                   |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.12 自检告警巡视详细信息段

自检告警巡视详细信息段（\<SelfAlarm>节点）描述了站内所有已巡视装置的自检告警巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RealVal     | 实际值                 | INT8          | 0：未告警（复归）；1：告警                   |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.13 时钟巡视详细信息段

时钟巡视详细信息段（\<DiffClock>节点）描述了站内所有已巡视装置的时钟巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                                   |
| :---------- | :------------------ | :------------ | :----------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                          |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference                  |
| PointName   | 信号点名称               | string（1-64）  |                                      |
| ValType     | 数值类型                | string（1-64）  | 本信息段的数值类型为datetime（日期时间）             |
| RefVal      | 标准值                 | datetime      | 巡视时本地的标准时刻, 格式为：yyyy-MM-ddThh:mm:ss  |
| RealVal     | 实际值                 | datetime      | 巡视时IED的实际时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss     |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                            |

<br />

##### A.4.3.14 对时告警巡视详细信息段

对时告警巡视详细信息段（\<GpsAlarm>节点）描述了站内所有已巡视装置的GPS告警巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RealVal     | 实际值                 | INT8          | 0：未告警（复归）；1：告警                   |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.15 录波巡视详细信息段

录波巡视详细信息段（\<Wave>节点）描述了站内所有已巡视装置的录波巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| RealVal     | 实际值                 | string（1-128） | 录波文件名                            |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否有差异               | BOOLEAN       | 0：指定时间段内无录波；1：指定时间段内有录波          |

<br />

##### A.4.3.16 保护动作巡视详细信息段

保护动作详细信息段（< OpCheck>节点）描述了站内所有已巡视装置的保护动作巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| PointId     | 信号点ID               | string（1-256） | 信号点的61850 reference              |
| PointName   | 信号点名称               | string（1-64）  |                                  |
| RealVal     | 实际值                 | INT8          | 0：保护动作复归；1：保护动作                  |
| Quality     | 告警点品质信息#13位码        | BITSTRING     | 和装置保持一致                          |
| OpTime      | 保护动作时刻              | datetime      | 保护动作的时刻, 格式为：yyyy-MM-ddhh:mm:ss  |
| RCDFile     | 保护动作关联的录波文件名        | STRING        |                                  |
| MIDFile     | 保护动作关联的中间节点名        | STRING        | <br />                           |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

<br />

##### A.4.3.17 CRC巡视详细信息段

保护动作详细信息段（\<CRC>节点）描述了站内所有已巡视装置的保护装置虚端子CRC巡视情况，其格式如下：

| 字段名         | 含义                  | 类型            | 说明                               |
| :---------- | :------------------ | :------------ | :------------------------------- |
| DeviceId    | IED ID，变电站内IED的唯一标识 | string（1-256） | 装置的IED Name                      |
| CRCVal      | CRC值                | STRING        |                                  |
| RefVal      | 标准值                 | STRING        |                                  |
| CheckTime   | 检查时刻                | datetime      | 巡视此项的时刻, 格式为：yyyy-MM-ddThh:mm:ss |
| IsDifferent | 是否异常                | BOOLEAN       | 0：正常；1：异常                        |

***

档版本：v1.0 | 编制日期：2026年7月
