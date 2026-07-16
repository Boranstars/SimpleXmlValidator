#include "gui/MainWindow.h"

#include "infrastructure/xerces/XercesRuntime.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <string>
#include <utility>

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

    auto* central = new QWidget(this);
    auto* layout  = new QVBoxLayout(central);

    // 顶部标题栏（应用内标题）。
    auto* titleLabel = new QLabel("XML 语法校验工具", central);
    QFont titleFont  = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    // XML 文件行：路径输入框在左、选择按钮在右；输入框同时作为拖放目标。
    xmlPathEdit_ = new QLineEdit(central);
    xmlPathEdit_->setReadOnly(true);
    xmlPathEdit_->setPlaceholderText("未选择 XML 文件（可拖放文件到此处）");
    xmlPathEdit_->setAcceptDrops(true);
    xmlPathEdit_->installEventFilter(this);
    selectXmlButton_ = new QPushButton("选择 XML 文件", central);
    auto* xmlRow     = new QHBoxLayout();
    xmlRow->addWidget(xmlPathEdit_, 1);
    xmlRow->addWidget(selectXmlButton_, 0);

    // XSD 文件行。
    xsdPathEdit_ = new QLineEdit(central);
    xsdPathEdit_->setReadOnly(true);
    xsdPathEdit_->setPlaceholderText("未选择 XSD 文件（可拖放文件到此处）");
    xsdPathEdit_->setAcceptDrops(true);
    xsdPathEdit_->installEventFilter(this);
    selectXsdButton_ = new QPushButton("选择 XSD 文件", central);
    auto* xsdRow     = new QHBoxLayout();
    xsdRow->addWidget(xsdPathEdit_, 1);
    xsdRow->addWidget(selectXsdButton_, 0);

    // 操作按钮行：开始校验 + 清空重置。
    validateButton_ = new QPushButton("开始校验", central);
    resetButton_    = new QPushButton("清空", central);
    auto* actionRow = new QHBoxLayout();
    actionRow->addWidget(validateButton_, 1);
    actionRow->addWidget(resetButton_, 0);

    // 结果提示条与错误明细区域，初始隐藏。
    bannerLabel_ = new QLabel(central);
    bannerLabel_->setAlignment(Qt::AlignCenter);
    bannerLabel_->setMinimumHeight(32);

    errorCountLabel_ = new QLabel(central);

    errorTable_ = new QTableWidget(central);
    errorTable_->setColumnCount(4);
    errorTable_->setHorizontalHeaderLabels(
        QStringList{"级别", "行号", "列号", "错误描述"});
    errorTable_->horizontalHeader()->setStretchLastSection(true);
    errorTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    errorTable_->setSelectionMode(QAbstractItemView::NoSelection);

    layout->addWidget(titleLabel);
    layout->addLayout(xmlRow);
    layout->addLayout(xsdRow);
    layout->addLayout(actionRow);
    layout->addWidget(bannerLabel_);
    layout->addWidget(errorCountLabel_);
    layout->addWidget(errorTable_, 1);

    setCentralWidget(central);

    connect(selectXmlButton_, &QPushButton::clicked, this, &MainWindow::onSelectXml);
    connect(selectXsdButton_, &QPushButton::clicked, this, &MainWindow::onSelectXsd);
    connect(validateButton_, &QPushButton::clicked, this, &MainWindow::onValidate);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::onReset);
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
        this, "选择 XML 文件", QString(),
        "XML 文件 (*.xml);;所有文件 (*)");
    // 用户取消对话框时不改变当前状态。
    if (selected.isEmpty()) {
        return;
    }
    setXmlPath(pathFromQString(selected));
}

void MainWindow::onSelectXsd() {
    const QString selected = QFileDialog::getOpenFileName(
        this, "选择 XSD 文件", QString(),
        "XSD 文件 (*.xsd);;所有文件 (*)");
    if (selected.isEmpty()) {
        return;
    }
    setXsdPath(pathFromQString(selected));
}

void MainWindow::setXmlPath(const std::filesystem::path& path) {
    xmlPath_ = path;
    xmlPathEdit_->setText(qStringFromPath(path));
    guiLog_.info("已选择 XML 文件");
    // 重新选择文件后清理旧结果，回到初始展示。
    clearResultArea();
    updateValidateButtonState();
}

void MainWindow::setXsdPath(const std::filesystem::path& path) {
    xsdPath_ = path;
    xsdPathEdit_->setText(qStringFromPath(path));
    guiLog_.info("已选择 XSD 文件");
    clearResultArea();
    updateValidateButtonState();
}

void MainWindow::updateValidateButtonState() {
    // XML 与 XSD 均已选择后才允许校验。
    validateButton_->setEnabled(!xmlPath_.empty() && !xsdPath_.empty());
}

void MainWindow::clearResultArea() {
    applyPresented(presenter_.initial());
}

void MainWindow::onReset() {
    // 清空已选择的 XML/XSD 文件、路径输入框与结果区域，回到初始状态。
    xmlPath_.clear();
    xsdPath_.clear();
    xmlPathEdit_->clear();
    xsdPathEdit_->clear();
    clearResultArea();
    updateValidateButtonState();
    guiLog_.info("已清空输入与结果");
}

void MainWindow::onValidate() {
    if (xmlPath_.empty() || xsdPath_.empty()) {
        return;
    }

    guiLog_.info("用户发起校验");

    // 校验期间禁用按钮，避免重复提交；同步执行，完成后恢复。
    validateButton_->setEnabled(false);
    const ValidationResult result = validator_.validate(xmlPath_, xsdPath_);
    applyPresented(presenter_.present(result));
    updateValidateButtonState();
}

void MainWindow::applyPresented(const PresentedResult& presented) {
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

    // 错误数量与表格。
    if (presented.showErrorTable) {
        const std::size_t groupCount = presented.errorRows.size();
        QString           countText =
            QString("错误数量：%1").arg(static_cast<qulonglong>(presented.totalErrorCount));
        // 合并了完全相同的诊断时，说明表格按“类别”展示。
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

        errorTable_->setRowCount(static_cast<int>(presented.errorRows.size()));
        for (int row = 0; row < static_cast<int>(presented.errorRows.size()); ++row) {
            const PresentedError& error = presented.errorRows[static_cast<std::size_t>(row)];
            // 同位置同文本被合并时，在描述后标注出现次数。
            QString description = QString::fromStdString(error.message);
            if (error.occurrences > 1) {
                description += QString("（共 %1 处）")
                                   .arg(static_cast<qulonglong>(error.occurrences));
            }
            errorTable_->setItem(
                row, 0, new QTableWidgetItem(QString::fromStdString(error.severity)));
            errorTable_->setItem(
                row, 1, new QTableWidgetItem(QString::fromStdString(error.line)));
            errorTable_->setItem(
                row, 2, new QTableWidgetItem(QString::fromStdString(error.column)));
            errorTable_->setItem(row, 3, new QTableWidgetItem(description));
        }
        errorTable_->show();
    } else {
        errorCountLabel_->hide();
        errorCountLabel_->clear();
        errorTable_->clearContents();
        errorTable_->setRowCount(0);
        errorTable_->hide();
    }

    // 阻断性弹窗。
    if (presented.showBlockingDialog) {
        guiLog_.warning("校验阻断，弹出阻断性提示");
        QMessageBox::critical(
            this, "无法完成校验", QString::fromStdString(presented.dialogMessage));
    }
}

}  // namespace simple_xml_validator::gui
