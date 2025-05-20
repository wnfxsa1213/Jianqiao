#ifndef ADMINDASHBOARDVIEW_H
#define ADMINDASHBOARDVIEW_H

#include <QWidget>
#include <QLabel> // For placeholder
#include <QVBoxLayout> // For placeholder
#include <QPushButton> // Added for m_exitButton member
#include <QListWidget> // Added
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
    void onChangePasswordClicked(); // Added slot for password change
    // void onSomeSettingChanged();

private:
    void setupUi();
    void populateWhitelistView(); // Helper to refresh the list view

    // Placeholder UI elements
    QLabel *m_placeholderLabel;
    QVBoxLayout *m_mainLayout;
    QPushButton *m_exitButton;

    // Whitelist management UI
    QListWidget* m_whitelistListWidget; // Added
    QPushButton* m_addAppButton;        // Added
    QPushButton* m_removeAppButton;     // Added

    QList<AppInfo> m_currentApps; // Added to store current apps

    // Hotkey UI
    QLabel* m_currentHotkeyTitleLabel; // Added: e.g., "Current Admin Hotkey:"
    QLabel* m_currentHotkeyDisplayLabel; // Added: Displays the actual hotkey string
    QPushButton* m_editHotkeyButton;    // Added: Button to trigger hotkey edit dialog
    QPushButton* m_exitApplicationButton; // Added button member
    QPushButton* m_changePasswordButton; // Added button member for password change

    // Example: Pointers to specialized widgets if we break it down
    // WhitelistManagerWidget* m_whitelistWidget;
    // PasswordSettingsWidget* m_passwordWidget;
    // HotkeySettingsWidget* m_hotkeyWidget;
};

#endif // ADMINDASHBOARDVIEW_H 