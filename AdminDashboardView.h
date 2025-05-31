#ifndef ADMINDASHBOARDVIEW_H
#define ADMINDASHBOARDVIEW_H

#include <QWidget>
#include <QLabel> // For placeholder
#include <QVBoxLayout> // For placeholder
#include <QPushButton> // Added for m_exitButton member
#include <QListWidget> // Added
#include <QTabWidget> // <--- 添加 QTabWidget 头文件
#include <QLineEdit> // Added for QLineEdit
#include <windows.h> // Added for DWORD type
#include "common_types.h" // Corrected path
#include "DetectionResultDialog.h" // <<< Include DetectionResultDialog
#include <QSpinBox>
#include <QCheckBox> // 新增：用于自启动复选框
#include <QProgressDialog> // 新增：用于进度弹窗

// Forward declarations if needed
// class WhitelistManagerWidget; // If we decide to embed a refactored part
// class PasswordSettingsWidget;
// class HotkeySettingsWidget;
class SystemInteractionModule; // Forward declare
class UserModeModule; // Forward declare

class AdminDashboardView : public QWidget
{
    Q_OBJECT

public:
    // Modified constructor to accept SystemInteractionModule pointer
    explicit AdminDashboardView(SystemInteractionModule* systemInteractionModule, QWidget *parent = nullptr);
    ~AdminDashboardView();

    void setWhitelistedApps(const QList<AppInfo>& apps);
    void setCurrentAdminLoginHotkey(const QStringList& hotkeyStrings);
    // void setPasswordChangeInterface(); // Placeholder
    // void setHotkeySettingsInterface(); // Placeholder
    void setSystemInteractionModulePtr(SystemInteractionModule* ptr) { m_systemInteractionModulePtr = ptr; }
    void setUserModeModule(UserModeModule* ptr) { m_userModeModule = ptr; }

signals:
    void userRequestsExitAdminMode();
    void whitelistChanged(const QList<AppInfo>& updatedApps);
    void changePasswordRequested(const QString& currentPassword, const QString& newPassword);
    void adminLoginHotkeyChanged(const QList<DWORD>& newHotkeySequence); // Assuming AdminModule provides DWORD list
    // void viewClosed(); // Replaced by userRequestsExitAdminMode for clarity
    void detectionResultsReceived(const SuggestedWindowHints& hints, bool success, const QString& errorString); // <<< NEW SIGNAL
    void appTopmostCheckBoxChanged(int appIndex, bool smartChecked, bool forceChecked);

private slots:
    void onAddAppClicked();    // Added
    void onRemoveAppClicked(); // Added
    void onDetectAndAddAppClicked(); // <<< NEW SLOT for detection workflow
    void onChangeHotkeyClicked(); // Added slot for changing hotkey
    void onExitApplicationClicked(); // Added slot
    void onConfirmPasswordChangeClicked(); // <--- 新增槽函数
    // void onSomeSettingChanged();
    void onDetectionResultsReceived(const SuggestedWindowHints& hints, bool success, const QString& errorString); // <<< NEW SLOT
    void onDetectionDialogApplied(const QString& finalMainExecutableHint, const QJsonObject& finalWindowHints); // <<< NEW SLOT for dialog results
    void onDetectionWaitMsSaveClicked(); // 新增槽函数
    void onAutoStartCheckBoxToggled(bool checked); // 新增：自启动槽函数
    void onSmartTopmostCheckBoxToggled(bool checked); // 槽函数
    void onForceTopmostCheckBoxToggled(bool checked); // 槽函数
    void onAppTopmostCheckBoxChanged(int appIndex, bool smartChecked, bool forceChecked);

private:
    void setupUi();
    void populateWhitelistView(); // Helper to refresh the list view

    // Main layout and TabWidget
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget; // <--- 添加 QTabWidget 成员

    // Widgets for Tabs (will contain the groups)
    QWidget* m_whitelistTab;
    QWidget* m_settingsTab;

    // Whitelist management UI (will be on m_whitelistTab)
    QListWidget* m_whitelistListWidget;
    QPushButton* m_addAppButton;
    QPushButton* m_detectAndAddAppButton; // <<< NEW BUTTON for detection
    QPushButton* m_removeAppButton;
    QList<AppInfo> m_currentApps;

    // Settings UI (will be on m_settingsTab)
    // Hotkey UI
    QLabel* m_currentHotkeyTitleLabel;
    QLabel* m_currentHotkeyDisplayLabel;
    QPushButton* m_editHotkeyButton;
    
    // Password UI
    // QPushButton* m_changePasswordButton; // This button will be replaced by a dedicated section
    QLabel* m_currentPasswordLabel;
    QLineEdit* m_currentPasswordLineEdit;
    QLabel* m_newPasswordLabel;
    QLineEdit* m_newPasswordLineEdit;
    QLabel* m_confirmPasswordLabel;
    QLineEdit* m_confirmPasswordLineEdit;
    QPushButton* m_confirmChangePasswordButton;

    // Buttons at the bottom
    QPushButton *m_exitButton;
    QPushButton* m_exitApplicationButton;

    // Placeholder UI elements
    // QLabel *m_placeholderLabel; // No longer needed with tabs

    // Example: Pointers to specialized widgets if we break it down
    // WhitelistManagerWidget* m_whitelistWidget;
    // PasswordSettingsWidget* m_passwordWidget;
    // HotkeySettingsWidget* m_hotkeyWidget;

    SystemInteractionModule* m_systemInteractionModulePtr = nullptr; // 用于安全退出时卸载钩子
    UserModeModule* m_userModeModule = nullptr; // 用于安全退出时终止子进程
    QString m_pendingDetectionAppPath; // To store path while waiting for detection results
    QString m_pendingDetectionAppName; // To store name while waiting for detection results
    QSpinBox* m_detectionWaitMsSpinBox; // 探测等待时间设置
    QPushButton* m_saveDetectionWaitMsButton; // 保存按钮

    QCheckBox* m_autoStartCheckBox = nullptr; // 新增：自启动复选框
    QCheckBox* m_smartTopmostCheckBox = nullptr; // 智能置顶复选框
    QCheckBox* m_forceTopmostCheckBox = nullptr; // 强力置顶复选框
    void updateAutoStartCheckBoxState(); // 新增：辅助函数
    void updateTopmostCheckBoxState(); // 辅助函数

    QProgressDialog* m_detectionProgressDialog; // 新增：探测进度弹窗指针

    bool isSmartTopmostEnabled() const;
    bool isForceTopmostEnabled() const;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // ADMINDASHBOARDVIEW_H 