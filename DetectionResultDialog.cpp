#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QJsonArray>
#include "common_types.h" // For struct SuggestedWindowHints
#include "SystemInteractionModule.h" // For WindowCandidateInfo
#include "DetectionResultDialog.h"
#include <QDebug>
#include <QFrame> // For QFrame line separator

DetectionResultDialog::DetectionResultDialog(const SuggestedWindowHints& initialHints, QWidget *parent)
    : QDialog(parent),
      m_mainExecutableLineEdit(nullptr),
      m_classNameLineEdit(nullptr),
      m_windowTitleLineEdit(nullptr),
      m_allowNonTopLevelCheckBox(nullptr),
      m_minScoreSpinBox(nullptr),
      m_applyButton(nullptr),
      m_cancelButton(nullptr),
      m_buttonBox(nullptr),
      m_initialHints(initialHints)
{
    // 自动解析initialHints.candidatesJson为m_candidates
    m_candidates.clear();
    for (const QJsonValue& val : initialHints.candidatesJson) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        WindowCandidateInfo info;
        info.hwnd = reinterpret_cast<HWND>(obj.value("hwnd").toString().toULongLong());
        info.className = obj.value("className").toString();
        info.title = obj.value("title").toString();
        info.isVisible = obj.value("isVisible").toBool();
        info.isTopLevel = obj.value("isTopLevel").toBool();
        info.processId = obj.value("processId").toInt();
        info.score = obj.value("score").toInt();
        m_candidates.append(info);
    }
    setupUi(initialHints);
    setWindowTitle(tr("探测结果与参数建议"));
    setMinimumWidth(600);
}

DetectionResultDialog::~DetectionResultDialog()
{
    qDebug() << "DetectionResultDialog destroyed.";
}

void DetectionResultDialog::setupUi(const struct SuggestedWindowHints& initialHints)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setRowWrapPolicy(QFormLayout::WrapAllRows);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setSpacing(10);

    // 新增：探测失败时，顶部显示红色失败原因
    if (!initialHints.isValid && !initialHints.errorString.isEmpty()) {
        QLabel* errorLabel = new QLabel(tr("失败原因：") + initialHints.errorString, this);
        errorLabel->setStyleSheet("color: #d32f2f; font-weight: bold; font-size: 14px;");
        mainLayout->addWidget(errorLabel);
    }

    QLabel* infoLabel = new QLabel(tr("以下是根据选择的应用程序探测到的参数建议。您可以修改这些参数后再应用："), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    m_mainExecutableLineEdit = new QLineEdit(initialHints.detectedMainExecutableName, this);
    m_mainExecutableLineEdit->setPlaceholderText(tr("例如 wps.exe, 空表示与启动器相同"));
    formLayout->addRow(tr("主程序文件名 (可选):    "), m_mainExecutableLineEdit);

    m_classNameLineEdit = new QLineEdit(initialHints.detectedClassName, this);
    m_classNameLineEdit->setPlaceholderText(tr("例如 OpusApp, Chrome_WidgetWin_1"));
    formLayout->addRow(tr("窗口类名 (主要):    "), m_classNameLineEdit);

    m_windowTitleLineEdit = new QLineEdit(initialHints.exampleTitle, this);
    m_windowTitleLineEdit->setPlaceholderText(tr("窗口标题包含的关键词 (可选)"));
    formLayout->addRow(tr("窗口标题关键词 (可选): "), m_windowTitleLineEdit);

    m_minScoreSpinBox = new QSpinBox(this);
    m_minScoreSpinBox->setRange(0, 200);
    if (initialHints.isValid && initialHints.bestScoreDuringDetection >= 0) {
        m_minScoreSpinBox->setValue(initialHints.bestScoreDuringDetection);
    } else {
        m_minScoreSpinBox->setValue(50);
    }
    formLayout->addRow(tr("窗口查找最小得分: "), m_minScoreSpinBox);

    m_allowNonTopLevelCheckBox = new QCheckBox(tr("允许非顶层窗口 (例如某些应用的子窗口是主交互窗口)"), this);
    m_allowNonTopLevelCheckBox->setChecked(!initialHints.isTopLevel && initialHints.isValid);
    if (!initialHints.isValid) m_allowNonTopLevelCheckBox->setChecked(false);
    formLayout->addRow(QString(), m_allowNonTopLevelCheckBox);

    mainLayout->addLayout(formLayout);

    // 新增：详细参数展示区，展示所有新采集的窗口和进程属性
    QGroupBox* detailGroup = new QGroupBox(tr("窗口与进程详细参数"), this);
    QVBoxLayout* detailVLayout = new QVBoxLayout();
    // 1. 展示主窗口参数（如有）
    QFormLayout* detailLayout = new QFormLayout();
    detailLayout->addRow(tr("进程完整路径："), new QLabel(initialHints.processFullPath, this));
    detailLayout->addRow(tr("父进程ID："), new QLabel(QString::number(initialHints.parentProcessId), this));
    detailLayout->addRow(tr("父窗口句柄："), new QLabel(QString::number(reinterpret_cast<quintptr>(initialHints.parentWindowHandle)), this));
    detailLayout->addRow(tr("窗口层级："), new QLabel(QString::number(initialHints.windowHierarchyLevel), this));
    detailLayout->addRow(tr("是否可见："), new QLabel(initialHints.isVisible ? tr("是") : tr("否"), this));
    detailLayout->addRow(tr("是否最小化："), new QLabel(initialHints.isMinimized ? tr("是") : tr("否"), this));
    detailVLayout->addLayout(detailLayout);
    // 2. 展示所有候选窗口详细参数（如有）
    if (!m_candidates.isEmpty()) {
        QLabel* allLabel = new QLabel(tr("所有候选窗口详细参数："), this);
        allLabel->setStyleSheet("color: #1976d2; font-weight: bold;");
        detailVLayout->addWidget(allLabel);
        QTableWidget* detailTable = new QTableWidget(m_candidates.size(), 7, this);
        detailTable->setHorizontalHeaderLabels(QStringList()
            << tr("窗口句柄") << tr("进程ID") << tr("父窗口句柄")
            << tr("类名") << tr("标题") << tr("可见") << tr("顶层"));
        detailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        for (int i = 0; i < m_candidates.size(); ++i) {
            const WindowCandidateInfo& info = m_candidates[i];
            detailTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg((quintptr)info.hwnd, 0, 16)));
            detailTable->setItem(i, 1, new QTableWidgetItem(QString::number(info.processId)));
            detailTable->setItem(i, 2, new QTableWidgetItem(QString("0x%1").arg((quintptr)info.hwnd, 0, 16)));
            detailTable->setItem(i, 3, new QTableWidgetItem(info.className));
            detailTable->setItem(i, 4, new QTableWidgetItem(info.title));
            detailTable->setItem(i, 5, new QTableWidgetItem(info.isVisible ? tr("是") : tr("否")));
            detailTable->setItem(i, 6, new QTableWidgetItem(info.isTopLevel ? tr("是") : tr("否")));
        }
        detailVLayout->addWidget(detailTable);
    }
    detailGroup->setLayout(detailVLayout);
    mainLayout->addWidget(detailGroup);

    // 探测失败时，展示候选窗口列表
    if (!initialHints.isValid && !m_candidates.isEmpty()) {
        QLabel* candidateLabel = new QLabel(tr("未能自动探测到主窗口，请从下方候选窗口中选择："), this);
        candidateLabel->setStyleSheet("color: #d9534f; font-weight: bold;");
        mainLayout->addWidget(candidateLabel);

        // 创建表格展示候选窗口
        QTableWidget* table = new QTableWidget(m_candidates.size(), 6, this);
        table->setHorizontalHeaderLabels(QStringList() << tr("类名") << tr("标题") << tr("可见性") << tr("顶层") << tr("分数") << tr("进程ID"));
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        for (int i = 0; i < m_candidates.size(); ++i) {
            const WindowCandidateInfo& info = m_candidates[i];
            table->setItem(i, 0, new QTableWidgetItem(info.className));
            table->setItem(i, 1, new QTableWidgetItem(info.title));
            table->setItem(i, 2, new QTableWidgetItem(info.isVisible ? tr("是") : tr("否")));
            table->setItem(i, 3, new QTableWidgetItem(info.isTopLevel ? tr("是") : tr("否")));
            table->setItem(i, 4, new QTableWidgetItem(QString::number(info.score)));
            table->setItem(i, 5, new QTableWidgetItem(QString::number(info.processId)));
        }
        mainLayout->addWidget(table);
        // 新增：双击行直接应用参数
        connect(table, &QTableWidget::cellDoubleClicked, this, [=](int row, int /*col*/){
            const WindowCandidateInfo& info = m_candidates[row];
            m_classNameLineEdit->setText(info.className);
            m_windowTitleLineEdit->setText(info.title);
            m_allowNonTopLevelCheckBox->setChecked(!info.isTopLevel);
            m_minScoreSpinBox->setValue(info.score);
            onApplyClicked(); // 直接应用
        });
        // 新增：表格下方增加说明
        QLabel* tipLabel = new QLabel(tr("点击表格某一行可自动填充参数，确认无误后点击下方'应用参数'保存。"), this);
        tipLabel->setStyleSheet("color: #1976d2; font-size: 12px;");
        mainLayout->addWidget(tipLabel);
        // 选中行时自动填充参数
        connect(table, &QTableWidget::cellClicked, this, [=](int row, int /*col*/){
            // 中文注释：点击候选窗口行，自动填充参数输入框
            const WindowCandidateInfo& info = m_candidates[row];
            m_classNameLineEdit->setText(info.className);
            m_windowTitleLineEdit->setText(info.title);
            m_allowNonTopLevelCheckBox->setChecked(!info.isTopLevel);
            m_minScoreSpinBox->setValue(info.score);
        });
    }

    // Add a separator
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_applyButton = m_buttonBox->button(QDialogButtonBox::Ok);
    m_applyButton->setText(tr("应用参数"));
    m_cancelButton = m_buttonBox->button(QDialogButtonBox::Cancel);
    m_cancelButton->setText(tr("取消"));

    connect(m_applyButton, &QPushButton::clicked, this, &DetectionResultDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &DetectionResultDialog::reject);

    mainLayout->addWidget(m_buttonBox);
    setLayout(mainLayout);

    // 探测失败时，清空参数输入框
    if (!initialHints.isValid) {
        m_mainExecutableLineEdit->clear();
        m_classNameLineEdit->clear();
        m_windowTitleLineEdit->clear();
        m_allowNonTopLevelCheckBox->setChecked(false);
        infoLabel->setText(tr("未能自动探测到明确的参数。请尝试手动填写或检查应用兼容性。"));
    }

    // 若m_candidates为空，增加提示
    if (m_candidates.isEmpty()) {
        QLabel* emptyLabel = new QLabel(tr("未采集到任何候选窗口，请检查应用是否正常启动或参数设置是否合理。"), this);
        emptyLabel->setStyleSheet("color: #d32f2f; font-weight: bold;");
        mainLayout->addWidget(emptyLabel);
    }
}

void DetectionResultDialog::onApplyClicked()
{
    QString finalMainExecutable = m_mainExecutableLineEdit->text().trimmed();
    QString finalClassName = m_classNameLineEdit->text().trimmed();
    QString finalTitleContains = m_windowTitleLineEdit->text().trimmed();
    bool finalAllowNonTopLevel = m_allowNonTopLevelCheckBox->isChecked();

    // Basic validation: at least class name or title should be present if it's not a simple launcher override
    if (finalClassName.isEmpty() && finalTitleContains.isEmpty() && finalMainExecutable.isEmpty()) {
        // It's possible to only have mainExecutableHint, so this validation might be too strict.
        // Let's refine: if mainExecutableHint is present, others can be empty.
        // If mainExecutableHint is empty, then class or title should be present for window finding.
    }
    if (finalClassName.isEmpty() && finalTitleContains.isEmpty() && finalMainExecutable.compare(m_initialHints.detectedExecutableName, Qt::CaseInsensitive) == 0 ){
         // If only executable name is same as launched one, and no other hints, it's probably not enough for complex cases
    }

    QJsonObject windowHintsJson;
    if (!finalClassName.isEmpty()) {
        windowHintsJson["primaryClassName"] = finalClassName;
    }
    if (!finalTitleContains.isEmpty()) {
        windowHintsJson["titleContains"] = finalTitleContains;
    }
    windowHintsJson["allowNonTopLevel"] = finalAllowNonTopLevel;

    // Determine a minScore. This can be a fixed default or based on which fields are filled.
    // For now, a simple heuristic:
    int minScore = m_minScoreSpinBox->value();

    windowHintsJson["minScore"] = minScore; 

    qDebug() << "DetectionResultDialog: Applying - MainExe:" << finalMainExecutable 
             << "ClassName:" << finalClassName << "TitleContains:" << finalTitleContains
             << "AllowNonTop:" << finalAllowNonTopLevel << "MinScore:" << minScore;

    emit suggestionsApplied(finalMainExecutable, windowHintsJson);
    accept(); // Close the dialog with QDialog::Accepted status
}

QString DetectionResultDialog::getFinalMainExecutableHint() const
{
    if (m_mainExecutableLineEdit) return m_mainExecutableLineEdit->text().trimmed();
    return QString();
}

QJsonObject DetectionResultDialog::getFinalWindowHints() const
{
    QJsonObject hints;
    if (m_classNameLineEdit) {
        QString className = m_classNameLineEdit->text().trimmed();
        if (!className.isEmpty()) hints["primaryClassName"] = className;
    }
    if (m_windowTitleLineEdit) {
        QString title = m_windowTitleLineEdit->text().trimmed();
        if (!title.isEmpty()) hints["titleContains"] = title;
    }
    if (m_allowNonTopLevelCheckBox) {
        hints["allowNonTopLevel"] = m_allowNonTopLevelCheckBox->isChecked();
    }
    // Reconstruct minScore based on the logic in onApplyClicked or store it temporarily
    // For simplicity, let's use a default if not easily reconstructable here.
    // Or, the caller can reconstruct minScore from these basic hints if needed.
    // For now, this getter focuses on the direct editable fields.
    if (m_minScoreSpinBox) {
        hints["minScore"] = m_minScoreSpinBox->value();
    } else {
        int minScore = 30;
        if (hints.contains("primaryClassName")) minScore = 50;
        if (hints.contains("titleContains") && !hints.contains("primaryClassName")) minScore = 40;
        if (hints.contains("primaryClassName") && hints.contains("titleContains")) minScore = 70;
        if (getFinalMainExecutableHint().isEmpty() && !hints.contains("primaryClassName") && !hints.contains("titleContains")) minScore = 0;
        hints["minScore"] = minScore;
    }
    return hints;
} 