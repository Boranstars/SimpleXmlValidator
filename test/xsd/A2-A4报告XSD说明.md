# A2-A4 报告 XSD 说明

本目录放置 A.2 到 A.4 三类报告的 XML Schema 草案，共 4 个 XSD 文件：

| 文件 | 对应报告 | 根节点 | 用途 |
| --- | --- | --- | --- |
| `common_types.xsd` | 公共类型 | 无业务根节点 | 定义通用时间、0/1 布尔值、整数文本、浮点文本、公共属性组 |
| `setting_check.xsd` | A.2 定值智能比对报告 | `APP` | 校验定值校核记录、当前定值点、基准模板和比对结论 |
| `mnt_rpt.xsd` | A.3 二次安措防误报告 | `MntRpt` | 校验检修安措基本信息、变电站、间隔、装置和操作步骤 |
| `check_report.xsd` | A.4 智能巡视报告 | `CheckReport` | 校验巡视基本信息、IED 巡视概要和各类巡视明细 |

## A.2 定值智能比对报告

A.2 报告描述某个应用程序对装置定值的校核结果。文件名规则为 `SettingCheck_AppName_时间信息_比对结果.xml`，但 XSD 只能校验 XML 内容，不能校验文件名；文件名中的 `OK` / `ERR` 应由程序用正则单独检查。

核心结构是：

```xml
<APP appname="" appdesc="">
  <QueryTime time="">
    <IED name="" desc="">
      <Record type="Setting" time="">
        <Point path="" desc="" other="" />
      </Record>
      <CompareTemplate>
        <Point path="" desc="" other="" />
      </CompareTemplate>
      <Result>...</Result>
    </IED>
  </QueryTime>
</APP>
```

转换思路：`APP` 是根节点；`QueryTime` 表示校核时间；每个 `IED` 表示一个被校核装置；`Record` 是本次校核记录；`CompareTemplate` 是基准模板；`Result` 是最终结论。`Point` 下统一要求至少一个 `Zone`，以覆盖附录中的定值区结构。

## A.3 二次安措防误报告

A.3 报告描述一次检修安措操作及其防误评价。文件名规则为 `MntRpt_yyyyMMdd_hhmmss.xml`，同样需要由程序检查。

核心结构是：

```xml
<MntRpt>
  <Base time="" type="" objName="" total_estimate="" />
  <Substation name="" desc="" />
  <Bay name="" desc="" />
  <Ied name="" desc="" />
  <Step NO="" ied_name="" ied_desc="" estimate="" error_reason="" type="" object_path="" value="">
    <content>...</content>
  </Step>
</MntRpt>
```

转换思路：`Base` 是报告基本信息；`Substation` 是变电站信息；`Bay` 是可选间隔；`Ied` 是检修装置，可出现多次；`Step` 是安措步骤。`Step` 按附录要求为必填且至少一个。

注意：附录中的 `error_ reason` 带空格，XML 属性名不能包含空格，这里统一修正为 `error_reason`。

## A.4 智能巡视报告

A.4 报告描述一次智能巡视的完整结果。文件名规则为 `checkreport_yyyyMMdd_hhmmss.xml`，需要由程序检查。

核心结构分三部分：

```xml
<CheckReport>
  <System>...</System>
  <Ied>...</Ied>
  <DiffZone>...</DiffZone>
  <CommStatus>...</CommStatus>
  <SecCircuit>...</SecCircuit>
  ...
</CheckReport>
```

`System` 是巡视基本信息，包括厂站名、巡视时间、巡视原因、装置总数、巡视装置数、异常装置数。

`Ied` 是巡视概要结果，内部每个 `Item` 表示一台装置，`Zone`、`Setting`、`SoftPlate`、`CommStatus` 等子节点表示各巡视项的概要状态。

`Ied` 之后是详细巡视信息段，例如 `DiffZone`、`DiffSetting`、`CommStatus`、`SecCircuit`、`OpCheck`、`CRCCheck`、`SoftVersion`、`Wave`、`DiffAnalog`、`Sample`、`DiffCurrent`、`Loop`、`Channel` 等。它们大多由多个 `Item` 组成，只是属性集合略有差异。

转换思路：A.4 节点类型多、重复结构多，所以 `check_report.xsd` 抽出了多种 section 类型，例如告警类、模拟量类、比对值类、版本类、录波类、CRC 类等。详细段使用 `xs:choice`，允许这些详细节点按实际文件出现，这比硬编码所有节点顺序更适合现有样例和后续扩展。

## 主要注意事项

1. XSD 校验 XML 内容，不校验文件名。A.2、A.3、A.4 的文件名规则应在 C++ 程序中用正则校验。
2. 表格中的 `M/O` 对应 XSD 中的 `minOccurs` 或属性的 `use`；`≥1` 对应 `maxOccurs="unbounded"`。
3. 当前 XSD 以附录为准，并补充了少量实现上必要的约束，例如统一时间格式、`0/1` 布尔值、整数/字符串长度等。
4. 时间统一使用 `reportDateTimeType`，支持 `yyyy-MM-dd hh:mm:ss`、`yyyy-MM-dd hh:mm:ss.SSS`、`yyyy-MM-ddThh:mm:ss` 三类形式。
5. `0/1` 类布尔值统一用 `bool01Type`，例如 `IsDifferent`、`IsChecked`、`total_estimate`、`estimate`、`value`。
