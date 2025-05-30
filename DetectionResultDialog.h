#ifndef DETECTIONRESULTDIALOG_H
#define DETECTIONRESULTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QSpinBox>
#include "common_types.h" // For SuggestedWindowHints
#include "SystemInteractionModule.h" // 为WindowCandidateInfo结构体

class DetectionResultDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数只需传递initialHints，候选窗口自动解析
    DetectionResultDialog(const SuggestedWindowHints& initialHints, QWidget *parent = nullptr);
    ~DetectionResultDialog();

    // Getter for the finalized parameters (alternative to signal, or used by signal)
    QString getFinalMainExecutableHint() const;
    QJsonObject getFinalWindowHints() const;

signals:
    // Emitted when apply is clicked and data is ready
    void suggestionsApplied(const QString& finalMainExecutableHint, const QJsonObject& finalWindowHints);

private slots:
    void onApplyClicked();
    // No specific slot for cancel, QDialogButtonBox handles reject

private:
    void setupUi(const SuggestedWindowHints& initialHints);

    // UI Elements
    QLineEdit* m_mainExecutableLineEdit;
    QLineEdit* m_classNameLineEdit;
    QLineEdit* m_windowTitleLineEdit;
    QCheckBox* m_allowNonTopLevelCheckBox;
    QSpinBox*  m_minScoreSpinBox;

    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QDialogButtonBox* m_buttonBox;

    // Store initial hints in case they are needed for reset or comparison
    SuggestedWindowHints m_initialHints;

    // 新增：候选窗口信息列表
    QList<WindowCandidateInfo> m_candidates; // 由initialHints.candidatesJson自动解析
};

#endif // DETECTIONRESULTDIALOG_H 