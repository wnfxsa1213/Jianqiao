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
      m_initialHints(initialHints) // Store initial hints
{
    setupUi(initialHints);
    setWindowTitle(tr("探测结果与参数建议"));
    setMinimumWidth(450);
}

DetectionResultDialog::~DetectionResultDialog()
{
    qDebug() << "DetectionResultDialog destroyed.";
}

void DetectionResultDialog::setupUi(const SuggestedWindowHints& initialHints)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setRowWrapPolicy(QFormLayout::WrapAllRows);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setSpacing(10);

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
    // Or directly use QDialogButtonBox signals:
    // connect(m_buttonBox, &QDialogButtonBox::accepted, this, &DetectionResultDialog::onApplyClicked);
    // connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_buttonBox);
    setLayout(mainLayout);

    // Initial population of fields
    if (!initialHints.isValid) {
        // If initial detection failed, disable some fields or show a message
        m_mainExecutableLineEdit->clear();
        m_classNameLineEdit->clear();
        m_windowTitleLineEdit->clear();
        m_allowNonTopLevelCheckBox->setChecked(false);
        infoLabel->setText(tr("未能自动探测到明确的参数。请尝试手动填写或检查应用兼容性。"));
        // Optionally disable fields if detection truly failed and we want to force manual input
        // m_classNameLineEdit->setEnabled(false);
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