#include "gui/MainWindow.h"

#include "gui/CodeEditor.h"
#include "gui/XmlSyntaxHighlighter.h"
#include "infrastructure/xerces/XercesRuntime.h"

#include <QAction>
#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace simple_xml_validator::gui {
namespace {

namespace logging_ns = simple_xml_validator::infrastructure::logging;

// Qt 与 std::filesystem::path 之间经 UTF-16 转换，跨平台支持中文与含空格路径。
std::filesystem::path pathFromQString(const QString& value) {
    const std::u16string utf16(value.utf16(), value.utf16() + value.size());
    return std::filesystem::path(utf16);
}

QString qStringFromPath(const std::filesystem::path& path) {
    const std::u16string utf16 = path.u16string();
    return QString::fromUtf16(utf16.data(), static_cast<int>(utf16.size()));
}

// 从拖放数据中提取唯一的本地文件路径；不满足条件时返回空。
QString firstLocalFile(const QMimeData* mimeData) {
    if (mimeData == nullptr || !mimeData->hasUrls()) {
        return QString();
    }
    const QList<QUrl> urls = mimeData->urls();
    if (urls.size() != 1 || !urls.front().isLocalFile()) {
        return QString();
    }
    return urls.front().toLocalFile();
}

logging_ns::LogModule makeGuiLog(logging_ns::LogManager* logManager) {
    if (logManager == nullptr) {
        return logging_ns::LogModule{};
    }
    return logManager->module("Gui");
}

// 读取文本文件的全部行（按 UTF-8），失败时返回空。仅用于 GUI 展示（预览/上下文）。
std::vector<std::string> readLines(const std::filesystem::path& path) {
    std::vector<std::string> lines;
    std::ifstream            input(path, std::ios::binary);
    if (!input.is_open()) {
        return lines;
    }
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

// 创建一个状态卡片（标题 + 数值）。返回卡片框，并通过 valueOut 输出数值标签。
QFrame* makeCard(const QString& title, QLabel** valueOut, QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName("card");
    card->setFrameShape(QFrame::NoFrame);
    // 柔和投影：QSS 不支持 box-shadow，改用图形效果实现。
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 38));
    card->setGraphicsEffect(shadow);
    auto* layout    = new QVBoxLayout(card);
    auto* titleText = new QLabel(title, card);
    titleText->setStyleSheet("color: #6b7280;");
    auto* valueText = new QLabel("-", card);
    QFont valueFont = valueText->font();
    valueFont.setPointSize(valueFont.pointSize() + 4);
    valueFont.setBold(true);
    valueText->setFont(valueFont);
    layout->addWidget(titleText);
    layout->addWidget(valueText);
    *valueOut = valueText;
    return card;
}

}  // namespace

MainWindow::MainWindow(
    const infrastructure::xerces::XercesRuntime& runtime,
    logging_ns::LogManager*                      logManager,
    QWidget*                                     parent)
    : QMainWindow(parent),
      guiLog_(makeGuiLog(logManager)),
      validator_(runtime, logManager) {
    setupUi();
    updateValidateButtonState();
    clearResultArea();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("XML 语法校验工具");

    // 工具栏动作（同时挂到菜单栏与工具栏）。
    auto* openXmlAction  = new QAction("打开XML", this);
    auto* openXsdAction  = new QAction("打开XSD", this);
    validateAction_      = new QAction("开始校验", this);
    auto* clearAction    = new QAction("清空", this);
    connect(openXmlAction, &QAction::triggered, this, &MainWindow::onSelectXml);
    connect(openXsdAction, &QAction::triggered, this, &MainWindow::onSelectXsd);
    connect(validateAction_, &QAction::triggered, this, &MainWindow::onValidate);
    connect(clearAction, &QAction::triggered, this, &MainWindow::onReset);

    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction(openXmlAction);
    fileMenu->addAction(openXsdAction);
    fileMenu->addSeparator();
    fileMenu->addAction(validateAction_);
    fileMenu->addAction(clearAction);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close);
    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于", this, [this]() {
        QMessageBox::about(this, "关于",
                           "XML 语法校验工具\n检查 XML 格式良好性并执行 XSD Schema 校验。");
    });

    auto* toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);
    toolBar->addAction(openXmlAction);
    toolBar->addAction(openXsdAction);
    toolBar->addAction(validateAction_);
    toolBar->addAction(clearAction);

    // 主体三栏：文件列表 | 中间主区 | 错误详情。
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // 左：文件列表。
    auto* leftPanel  = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(new QLabel("文件列表", leftPanel));
    fileList_ = new QListWidget(leftPanel);
    leftLayout->addWidget(fileList_);
    splitter->addWidget(leftPanel);

    // 中：输入配置 + 状态卡片 + 结果多标签。
    auto* centerPanel  = new QWidget(splitter);
    auto* centerLayout = new QVBoxLayout(centerPanel);

    auto* inputGroup  = new QGroupBox("输入配置", centerPanel);
    auto* inputLayout = new QGridLayout(inputGroup);
    xmlPathEdit_ = new QLineEdit(inputGroup);
    xmlPathEdit_->setReadOnly(true);
    xmlPathEdit_->setPlaceholderText("未选择 XML 文件（可拖放文件到此处）");
    xmlPathEdit_->setAcceptDrops(true);
    xmlPathEdit_->installEventFilter(this);
    selectXmlButton_ = new QPushButton("浏览…", inputGroup);
    xsdPathEdit_ = new QLineEdit(inputGroup);
    xsdPathEdit_->setReadOnly(true);
    xsdPathEdit_->setPlaceholderText("未选择 XSD 文件（可拖放文件到此处）");
    xsdPathEdit_->setAcceptDrops(true);
    xsdPathEdit_->installEventFilter(this);
    selectXsdButton_ = new QPushButton("浏览…", inputGroup);
    validateButton_  = new QPushButton("开始校验", inputGroup);
    validateButton_->setObjectName("primaryButton");
    resetButton_     = new QPushButton("清空", inputGroup);
    inputLayout->addWidget(new QLabel("XML 文件路径：", inputGroup), 0, 0);
    inputLayout->addWidget(xmlPathEdit_, 0, 1);
    inputLayout->addWidget(selectXmlButton_, 0, 2);
    inputLayout->addWidget(new QLabel("XSD 文件路径：", inputGroup), 1, 0);
    inputLayout->addWidget(xsdPathEdit_, 1, 1);
    inputLayout->addWidget(selectXsdButton_, 1, 2);
    auto* actionRow = new QHBoxLayout();
    actionRow->addStretch(1);
    actionRow->addWidget(validateButton_);
    actionRow->addWidget(resetButton_);
    inputLayout->addLayout(actionRow, 2, 0, 1, 3);
    centerLayout->addWidget(inputGroup);

    auto* cardRow = new QHBoxLayout();
    cardRow->addWidget(makeCard("校验状态", &statusValueLabel_, centerPanel));
    cardRow->addWidget(makeCard("错误数量", &errorCountValueLabel_, centerPanel));
    cardRow->addWidget(makeCard("警告数量", &warningCountValueLabel_, centerPanel));
    cardRow->addWidget(makeCard("耗时", &elapsedValueLabel_, centerPanel));
    centerLayout->addLayout(cardRow);

    resultTabs_ = new QTabWidget(centerPanel);

    // 校验结果标签：提示条 + 数量说明 + 错误表格。
    auto* resultTab    = new QWidget(resultTabs_);
    auto* resultLayout = new QVBoxLayout(resultTab);
    bannerLabel_ = new QLabel(resultTab);
    bannerLabel_->setAlignment(Qt::AlignCenter);
    bannerLabel_->setMinimumHeight(28);
    errorCountLabel_ = new QLabel(resultTab);
    errorTable_      = new QTableWidget(resultTab);
    errorTable_->setColumnCount(5);
    errorTable_->setHorizontalHeaderLabels(
        QStringList{"级别", "文件", "行号", "列号", "错误描述"});
    errorTable_->horizontalHeader()->setStretchLastSection(true);
    errorTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    errorTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    errorTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    resultLayout->addWidget(bannerLabel_);
    resultLayout->addWidget(errorCountLabel_);
    resultLayout->addWidget(errorTable_, 1);
    resultTabs_->addTab(resultTab, "校验结果");

    // XML 预览标签（带行号与语法高亮）。
    xmlPreview_ = new CodeEditor(resultTabs_);
    previewHighlighter_ = new XmlSyntaxHighlighter(xmlPreview_->document());
    resultTabs_->addTab(xmlPreview_, "XML预览");

    // 错误日志标签（本次校验结果的文本化列表）。
    logView_ = new QPlainTextEdit(resultTabs_);
    logView_->setReadOnly(true);
    logView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    resultTabs_->addTab(logView_, "错误日志");

    // 统计信息标签。
    statsLabel_ = new QLabel(resultTabs_);
    statsLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    statsLabel_->setMargin(8);
    resultTabs_->addTab(statsLabel_, "统计信息");

    centerLayout->addWidget(resultTabs_, 1);
    splitter->addWidget(centerPanel);

    // 右：错误详情面板。
    auto* rightPanel  = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->addWidget(new QLabel("错误详情", rightPanel));
    detailLevelLabel_  = new QLabel("级别：-", rightPanel);
    detailFileLabel_   = new QLabel("文件：-", rightPanel);
    detailLineLabel_   = new QLabel("行：-", rightPanel);
    detailColumnLabel_ = new QLabel("列：-", rightPanel);
    rightLayout->addWidget(detailLevelLabel_);
    rightLayout->addWidget(detailFileLabel_);
    rightLayout->addWidget(detailLineLabel_);
    rightLayout->addWidget(detailColumnLabel_);
    rightLayout->addWidget(new QLabel("描述：", rightPanel));
    detailMessageEdit_ = new QPlainTextEdit(rightPanel);
    detailMessageEdit_->setReadOnly(true);
    detailMessageEdit_->setMaximumHeight(90);
    rightLayout->addWidget(detailMessageEdit_);
    rightLayout->addWidget(new QLabel("上下文：", rightPanel));
    contextEdit_ = new CodeEditor(rightPanel);
    contextHighlighter_ = new XmlSyntaxHighlighter(contextEdit_->document());
    rightLayout->addWidget(contextEdit_, 1);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    splitter->setStretchFactor(2, 2);
    setCentralWidget(splitter);

    statusBar()->showMessage("就绪");
    auto* versionLabel = new QLabel(QString("Qt %1 / Xerces-C++").arg(qVersion()), this);
    statusBar()->addPermanentWidget(versionLabel);

    connect(selectXmlButton_, &QPushButton::clicked, this, &MainWindow::onSelectXml);
    connect(selectXsdButton_, &QPushButton::clicked, this, &MainWindow::onSelectXsd);
    connect(validateButton_, &QPushButton::clicked, this, &MainWindow::onValidate);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::onReset);
    connect(errorTable_, &QTableWidget::itemSelectionChanged, this,
            &MainWindow::onErrorSelectionChanged);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == xmlPathEdit_ || watched == xsdPathEdit_) {
        if (event->type() == QEvent::DragEnter) {
            auto* dragEvent = static_cast<QDragEnterEvent*>(event);
            if (!firstLocalFile(dragEvent->mimeData()).isEmpty()) {
                // 不以扩展名硬性拒绝文件；真正的文件检查仍由 XmlValidator 完成。
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            auto*         dropEvent = static_cast<QDropEvent*>(event);
            const QString filePath  = firstLocalFile(dropEvent->mimeData());
            if (!filePath.isEmpty()) {
                if (watched == xmlPathEdit_) {
                    setXmlPath(pathFromQString(filePath));
                } else {
                    setXsdPath(pathFromQString(filePath));
                }
                dropEvent->acceptProposedAction();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onSelectXml() {
    const QString selected = QFileDialog::getOpenFileName(
        this, "选择 XML 文件", QString(), "XML 文件 (*.xml);;所有文件 (*)");
    // 用户取消对话框时不改变当前状态。
    if (selected.isEmpty()) {
        return;
    }
    setXmlPath(pathFromQString(selected));
}

void MainWindow::onSelectXsd() {
    const QString selected = QFileDialog::getOpenFileName(
        this, "选择 XSD 文件", QString(), "XSD 文件 (*.xsd);;所有文件 (*)");
    if (selected.isEmpty()) {
        return;
    }
    setXsdPath(pathFromQString(selected));
}

void MainWindow::setXmlPath(const std::filesystem::path& path) {
    xmlPath_ = path;
    xmlPathEdit_->setText(qStringFromPath(path));
    guiLog_.info("已选择 XML 文件");
    refreshFileList();
    loadXmlPreview();
    // 重新选择文件后清理旧结果，回到初始展示。
    clearResultArea();
    updateValidateButtonState();
}

void MainWindow::setXsdPath(const std::filesystem::path& path) {
    xsdPath_ = path;
    xsdPathEdit_->setText(qStringFromPath(path));
    guiLog_.info("已选择 XSD 文件");
    refreshFileList();
    clearResultArea();
    updateValidateButtonState();
}

void MainWindow::refreshFileList() {
    // 只读列出当前已选 XML/XSD，便于查看；不做多文件增删/批量。
    fileList_->clear();
    if (!xmlPath_.empty()) {
        fileList_->addItem(qStringFromPath(xmlPath_.filename()));
    }
    if (!xsdPath_.empty()) {
        fileList_->addItem(qStringFromPath(xsdPath_.filename()));
    }
}

void MainWindow::loadXmlPreview() {
    if (xmlPath_.empty()) {
        xmlPreview_->clear();
        return;
    }
    const std::vector<std::string> lines = readLines(xmlPath_);
    QString                        text;
    for (const std::string& line : lines) {
        text += QString::fromStdString(line);
        text += '\n';
    }
    xmlPreview_->setPlainText(text);
}

void MainWindow::updateValidateButtonState() {
    // XML 与 XSD 均已选择后才允许校验。
    const bool ready = !xmlPath_.empty() && !xsdPath_.empty();
    validateButton_->setEnabled(ready);
    if (validateAction_ != nullptr) {
        validateAction_->setEnabled(ready);
    }
}

void MainWindow::clearResultArea() {
    presented_ = presenter_.initial();
    applyPresented(presented_, 0.0);
}

void MainWindow::onReset() {
    // 清空已选择的 XML/XSD 文件、路径输入框、预览与结果区域，回到初始状态。
    xmlPath_.clear();
    xsdPath_.clear();
    xmlPathEdit_->clear();
    xsdPathEdit_->clear();
    refreshFileList();
    xmlPreview_->clear();
    clearResultArea();
    updateValidateButtonState();
    statusBar()->showMessage("就绪");
    guiLog_.info("已清空输入与结果");
}

void MainWindow::onValidate() {
    if (xmlPath_.empty() || xsdPath_.empty()) {
        return;
    }

    guiLog_.info("用户发起校验");

    // 校验期间禁用按钮，避免重复提交；同步执行，完成后恢复。
    validateButton_->setEnabled(false);
    if (validateAction_ != nullptr) {
        validateAction_->setEnabled(false);
    }
    statusBar()->showMessage("正在校验…");

    const auto start = std::chrono::steady_clock::now();
    const ValidationResult result = validator_.validate(xmlPath_, xsdPath_);
    const auto finish = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(finish - start).count();

    presented_ = presenter_.present(result);
    applyPresented(presented_, elapsedMs);
    updateValidateButtonState();
    statusBar()->showMessage("校验完成");
}

void MainWindow::applyPresented(const PresentedResult& presented, double elapsedMilliseconds) {
    // 提示条。
    switch (presented.banner) {
        case BannerKind::None:
            bannerLabel_->hide();
            bannerLabel_->clear();
            bannerLabel_->setStyleSheet(QString());
            break;
        case BannerKind::Success:
            bannerLabel_->setText(QString::fromStdString(presented.bannerText));
            bannerLabel_->setStyleSheet(
                "background-color: #2e7d32; color: white; font-weight: bold;");
            bannerLabel_->show();
            break;
        case BannerKind::Failure:
            bannerLabel_->setText(QString::fromStdString(presented.bannerText));
            bannerLabel_->setStyleSheet(
                "background-color: #c62828; color: white; font-weight: bold;");
            bannerLabel_->show();
            break;
    }

    // 状态卡片。
    switch (presented.statusCard) {
        case StatusCardKind::None:
            statusValueLabel_->setText("-");
            statusValueLabel_->setStyleSheet(QString());
            break;
        case StatusCardKind::Valid:
            statusValueLabel_->setText("校验通过");
            statusValueLabel_->setStyleSheet("color: #2e7d32;");
            break;
        case StatusCardKind::Invalid:
            statusValueLabel_->setText("校验未通过");
            statusValueLabel_->setStyleSheet("color: #c62828;");
            break;
        case StatusCardKind::Failed:
            statusValueLabel_->setText("校验失败");
            statusValueLabel_->setStyleSheet("color: #e65100;");
            break;
    }
    const bool hasResult = presented.statusCard != StatusCardKind::None;
    errorCountValueLabel_->setText(hasResult ? QString::number(
                                                   static_cast<qulonglong>(presented.errorCount))
                                             : QString("-"));
    warningCountValueLabel_->setText(hasResult ? QString::number(static_cast<qulonglong>(
                                                     presented.warningCount))
                                               : QString("-"));
    elapsedValueLabel_->setText(
        hasResult ? QString("%1 ms").arg(elapsedMilliseconds, 0, 'f', 1) : QString("-"));

    // 错误数量说明与表格。
    if (presented.showErrorTable) {
        const std::size_t groupCount = presented.errorRows.size();
        QString countText = QString("错误数量：%1  警告数量：%2")
                                .arg(static_cast<qulonglong>(presented.errorCount))
                                .arg(static_cast<qulonglong>(presented.warningCount));
        if (groupCount < presented.totalErrorCount) {
            countText += QString("（合并相同项后 %1 类）")
                             .arg(static_cast<qulonglong>(groupCount));
        }
        if (presented.truncated) {
            countText += QString("（仅显示前 %1 类，完整明细见日志）")
                             .arg(static_cast<qulonglong>(groupCount));
        }
        errorCountLabel_->setText(countText);
        errorCountLabel_->show();

        const QString fileName = qStringFromPath(xmlPath_.filename());
        errorTable_->setRowCount(static_cast<int>(groupCount));
        for (int row = 0; row < static_cast<int>(groupCount); ++row) {
            const PresentedError& error = presented.errorRows[static_cast<std::size_t>(row)];
            QString description = QString::fromStdString(error.message);
            if (error.occurrences > 1) {
                description += QString("（共 %1 处）")
                                   .arg(static_cast<qulonglong>(error.occurrences));
            }
            errorTable_->setItem(
                row, 0, new QTableWidgetItem(QString::fromStdString(error.severity)));
            errorTable_->setItem(row, 1, new QTableWidgetItem(fileName));
            errorTable_->setItem(
                row, 2, new QTableWidgetItem(QString::fromStdString(error.line)));
            errorTable_->setItem(
                row, 3, new QTableWidgetItem(QString::fromStdString(error.column)));
            errorTable_->setItem(row, 4, new QTableWidgetItem(description));
        }
        errorTable_->show();
    } else {
        errorCountLabel_->hide();
        errorCountLabel_->clear();
        errorTable_->clearContents();
        errorTable_->setRowCount(0);
        errorTable_->show();
    }

    // 错误日志文本视图（本次校验结果文本化）。
    if (hasResult) {
        const QString fileName = qStringFromPath(xmlPath_.filename());
        QString       log      = QString("校验状态：%1\n")
                          .arg(QString::fromStdString(presented.statusCardText));
        if (!presented.dialogMessage.empty()) {
            log += QString("说明：%1\n").arg(QString::fromStdString(presented.dialogMessage));
        }
        for (const PresentedError& error : presented.errorRows) {
            log += QString("[%1] %2 行 %3 列 %4 : %5")
                       .arg(QString::fromStdString(error.severity))
                       .arg(fileName)
                       .arg(QString::fromStdString(error.line))
                       .arg(QString::fromStdString(error.column))
                       .arg(QString::fromStdString(error.message));
            if (error.occurrences > 1) {
                log += QString("（共 %1 处）").arg(static_cast<qulonglong>(error.occurrences));
            }
            log += '\n';
        }
        logView_->setPlainText(log);

        statsLabel_->setText(
            QString("校验状态：%1\n错误数量（Error/Fatal）：%2\n警告数量（Warning）：%3\n"
                    "诊断总条数（合并前）：%4\n展示类别数（合并后）：%5\n耗时：%6 ms")
                .arg(QString::fromStdString(presented.statusCardText))
                .arg(static_cast<qulonglong>(presented.errorCount))
                .arg(static_cast<qulonglong>(presented.warningCount))
                .arg(static_cast<qulonglong>(presented.totalErrorCount))
                .arg(static_cast<qulonglong>(presented.errorRows.size()))
                .arg(elapsedMilliseconds, 0, 'f', 1));
    } else {
        logView_->clear();
        statsLabel_->clear();
    }

    // 清空错误详情面板。
    updateErrorDetail(-1);

    // 阻断性弹窗。
    if (presented.showBlockingDialog) {
        guiLog_.warning("校验阻断，弹出阻断性提示");
        QMessageBox::critical(
            this, "无法完成校验", QString::fromStdString(presented.dialogMessage));
    }
}

void MainWindow::onErrorSelectionChanged() {
    updateErrorDetail(errorTable_->currentRow());
}

void MainWindow::updateErrorDetail(int row) {
    if (row < 0 || row >= static_cast<int>(presented_.errorRows.size())) {
        detailLevelLabel_->setText("级别：-");
        detailFileLabel_->setText("文件：-");
        detailLineLabel_->setText("行：-");
        detailColumnLabel_->setText("列：-");
        detailMessageEdit_->clear();
        contextEdit_->clear();
        return;
    }

    const PresentedError& error = presented_.errorRows[static_cast<std::size_t>(row)];
    detailLevelLabel_->setText(
        QString("级别：%1").arg(QString::fromStdString(error.severity)));
    detailFileLabel_->setText(
        QString("文件：%1").arg(qStringFromPath(xmlPath_.filename())));
    detailLineLabel_->setText(QString("行：%1").arg(QString::fromStdString(error.line)));
    detailColumnLabel_->setText(QString("列：%1").arg(QString::fromStdString(error.column)));
    detailMessageEdit_->setPlainText(QString::fromStdString(error.message));

    // 上下文：读取 XML 源，显示出错行附近数行并高亮该行。只读展示，不提供跳转。
    contextEdit_->clear();
    contextEdit_->setFirstLineNumber(1);
    contextEdit_->setHighlightLine(-1);
    bool      ok     = false;
    const int lineNo = QString::fromStdString(error.line).toInt(&ok);
    if (!ok || lineNo <= 0 || xmlPath_.empty()) {
        return;
    }
    const std::vector<std::string> lines = readLines(xmlPath_);
    if (lines.empty()) {
        return;
    }
    const int total = static_cast<int>(lines.size());
    const int from  = std::max(1, lineNo - 3);
    const int to    = std::min(total, lineNo + 3);
    QString   context;
    for (int i = from; i <= to; ++i) {
        context += QString::fromStdString(lines[static_cast<std::size_t>(i - 1)]);
        if (i < to) {
            context += '\n';
        }
    }
    contextEdit_->setFirstLineNumber(from);
    contextEdit_->setPlainText(context);
    contextEdit_->setHighlightLine(lineNo);
}

}  // namespace simple_xml_validator::gui
