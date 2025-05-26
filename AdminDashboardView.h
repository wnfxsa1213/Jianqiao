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

// Forward declarations if needed
// class WhitelistManagerWidget; // If we decide to embed a refactored part
// class PasswordSettingsWidget;
// class HotkeySettingsWidget;

class AdminDashboardView : public QWidget
{
    Q_OBJECT

public:
    explicit AdminDashboardView(QWidget *parent = nullptr);
    ~AdminDashboardView();

    void setWhitelistedApps(const QList<AppInfo>& apps);
    void setCurrentAdminLoginHotkey(const QStringList& hotkeyStrings);
    // void setPasswordChangeInterface(); // Placeholder
    // void setHotkeySettingsInterface(); // Placeholder

signals:
    void userRequestsExitAdminMode();
    void whitelistChanged(const QList<AppInfo>& updatedApps);
    void changePasswordRequested(const QString& currentPassword, const QString& newPassword);
    void adminLoginHotkeyChanged(const QList<DWORD>& newHotkeySequence); // Assuming AdminModule provides DWORD list
    // void viewClosed(); // Replaced by userRequestsExitAdminMode for clarity

private slots:
    void onAddAppClicked();    // Added
    void onRemoveAppClicked(); // Added
    void onChangeHotkeyClicked(); // Added slot for changing hotkey
    void onExitApplicationClicked(); // Added slot
    void onConfirmPasswordChangeClicked(); // <--- 新增槽函数
    // void onSomeSettingChanged();

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
};

#endif // ADMINDASHBOARDVIEW_H 