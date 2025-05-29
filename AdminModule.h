#ifndef ADMINMODULE_H
#define ADMINMODULE_H

#include <QObject>
#include <QList> // Required for QList
#include "common_types.h" // Include common_types.h for WhitelistedApp
#include <Windows.h> // Added for DWORD
#include <QStringList>

// Forward declarations
class AdminLoginView;
class AdminDashboardView; // New dashboard view
class SystemInteractionModule;

// #include "WhitelistManagerView.h" // No longer needed just for WhitelistedApp struct
                                    // Keep if AdminModule needs full definition of WhitelistManagerView for other reasons
                                    // (e.g. member type, connecting signals/slots directly to its type)
                                    // For m_whitelistManagerView pointer, forward declaration is enough.

class AdminModule : public QObject
{
    Q_OBJECT
public:
    // explicit AdminModule(SystemInteractionModule* systemInteraction, QObject *parent = nullptr); // Old constructor
    explicit AdminModule(SystemInteractionModule* systemInteraction, AdminDashboardView* dashboardView, QObject *parent = nullptr); // New constructor
    ~AdminModule();

public slots:
    void showLoginView();
    void requestExitAdminMode(); // Called by CoreShell to hide all admin views
    void onAdminLoginHotkeyChanged(const QList<DWORD>& newHotkeySequence); // New: Changed parameter type
    bool isAnyViewVisible() const; // New method
    void onWhitelistUpdated(const QList<AppInfo>& updatedWhitelist); // <--- 修改参数类型
    QList<AppInfo> getWhitelistedApps() const;
    bool isLoginViewActive() const; // Added

private slots:
    void onUserRequestsExitAdminMode(); // ADDED new slot
    void onLoginViewRequestsExit(); // Slot for AdminLoginView exit requests
    void onLoginAttempt(const QString& password); // Slot for login attempts
    void onChangePasswordRequested(const QString& currentPassword, const QString& newPassword); // Slot for password change requests
    void onLoginViewHidden(); // Added slot to handle AdminLoginView hidden

signals:
    void adminViewVisible(bool visible);
    void exitAdminModeRequested(); // New signal
    void configurationChanged(); // For things like password or hotkey changes
    void loginSuccessfulAndAdminActive(); // Emitted when login is successful and admin can perform actions

private:
    AdminLoginView *m_loginView; 
    AdminDashboardView *m_adminDashboardView; // This will now hold the passed-in instance
    SystemInteractionModule* m_systemInteractionModulePtr; // Pointer to access system interaction functions
    QString m_adminPasswordHash; // Renamed from m_adminPassword to match usage in cpp
    QList<AppInfo> m_whitelistedApps; // <--- 修改成员变量类型
    QList<DWORD> m_currentAdminLoginHotkeyVkCodes; // Added declaration
    bool m_isAdminAuthenticated; // NEW: Tracks if admin is currently authenticated

    // Config related methods - now declared
    void loadConfig(); 
    void saveConfig(); // Assuming this will be implemented later or its usage removed if not needed
    void initializeDefaultAdminHotkey(); // Added declaration, needs implementation

    // Existing private methods, ensure they are all used or remove if obsolete
    void loadWhitelistFromConfig();
    bool saveWhitelistToConfig(const QList<AppInfo>& apps);
    void loadAdminPassword(); // This might be redundant if loadConfig handles it all
    bool verifyPassword(const QString& password);
    bool saveAdminPasswordToConfig(const QString& newPassword);
    bool saveAdminLoginHotkeyToConfig(const QStringList& hotkeyVkStrings);

    // void prepareAdminDashboardData(); // NEW private method - MOVING TO PUBLIC

public: // Ensure prepareAdminDashboardData is public if called from CoreShell
    void prepareAdminDashboardData(); // MOVED to public

    // 新增：统一获取配置文件路径的静态函数声明
    static QString getConfigFilePath();

};

#endif // ADMINMODULE_H 