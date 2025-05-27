#include "SystemInteractionModule.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication> // For applicationDirPath
#include <windows.h> // Ensure this is included for Windows API functions
#include <TlHelp32.h> // For process snapshot functions
#include <QWindow> // Required for QWindow::fromWinId()
#include <QTextStream> // Added for robust file reading
#include <QIcon>     // Added for getIconForExecutable
#include <QPixmap>   // Added for getIconForExecutable
#include <shellapi.h> // Added for SHGetFileInfo
#include <QDir>
#include <QStandardPaths>
#include <qt_windows.h>
#include <QtGui/QGuiApplication> // Added for Qt 6
#include <QtGui/QScreen> // Added for Qt 6 (though not directly used in this change, often needed with native interface)
#include <QtGui/qpa/qplatformnativeinterface.h> // Attempting include via qpa subdirectory
#include <QImage> // Ensure QImage is included for QImage::fromHICON
#include <string> // Added for std::wstring
#include <QTimer> // Make sure QTimer is included
#include <Psapi.h> // Required for EnumProcessModules, GetModuleFileNameEx - keep for now, assuming other functions might use it
#include <dwmapi.h> // For DWMWA_CLOAKED
#include <QFileInfo> // Required for QFileInfo to get base name
#include <QFileIconProvider> // Added for QFileIconProvider
#include <QThread> // 添加头文件
#include <VersionHelpers.h> // 确保包含了必要的头文件
#include <QCollator> // Qt5之后用于自然排序
#include <QtConcurrent> // Required for QtConcurrent::run
#include <QProcess> // Added for QProcess

// Define a structure to pass data to EnumWindowsProc for hint-based search
struct HintedEnumWindowsCallbackArg {
    DWORD targetPid;
    HWND bestHwnd;
    int bestScore; // This will store the score of the bestHwnd
    const QJsonObject* hints; // Pointer to the window hints

    HintedEnumWindowsCallbackArg(DWORD pid, const QJsonObject* h)
        : targetPid(pid), bestHwnd(nullptr), bestScore(-1), hints(h) {}
};

// Initialize static members
HHOOK SystemInteractionModule::keyboardHook_ = NULL;
SystemInteractionModule* SystemInteractionModule::instance_ = nullptr;
const QMap<QString, DWORD> SystemInteractionModule::VK_CODE_MAP = SystemInteractionModule::initializeVkCodeMap();

// Define constants for short activation
const int SHORT_ACTIVATION_ATTEMPTS = 5; // Max attempts
const int SHORT_ACTIVATION_INTERVAL_MS = 300; // Interval between attempts

// Define constants if not already defined elsewhere (e.g., at the top of the .cpp file)
// const int HINT_DETECTION_DELAY_MS = 5000; // Example, ensure this matches what's used

QMap<QString, DWORD> SystemInteractionModule::initializeVkCodeMap() {
    QMap<QString, DWORD> map;
    // Modifiers
    map.insert("VK_LCONTROL", VK_LCONTROL);   // 0xA2
    map.insert("VK_RCONTROL", VK_RCONTROL);   // 0xA3
    map.insert("VK_LSHIFT", VK_LSHIFT);     // 0xA0
    map.insert("VK_RSHIFT", VK_RSHIFT);     // 0xA1
    map.insert("VK_LMENU", VK_LMENU);       // 0xA4 (Left Alt)
    map.insert("VK_RMENU", VK_RMENU);       // 0xA5 (Right Alt)
    map.insert("VK_CONTROL", VK_CONTROL);   // Generic Control
    map.insert("VK_SHIFT", VK_SHIFT);       // Generic Shift
    map.insert("VK_MENU", VK_MENU);         // Generic Alt (Menu)
    // Letters
    for (char c = 'A'; c <= 'Z'; ++c) {
        map.insert(QString(c), c);
    }
    // Numbers
    for (char c = '0'; c <= '9'; ++c) {
        map.insert(QString(c), c);
    }
    // Function keys
    for (int i = 1; i <= 12; ++i) {
        map.insert(QString("F%1").arg(i), VK_F1 + (i - 1));
    }
    // Add other common keys as needed
    map.insert("VK_RETURN", VK_RETURN);     // Enter
    map.insert("VK_ESCAPE", VK_ESCAPE);   // Esc
    map.insert("VK_TAB", VK_TAB);         // Tab
    map.insert("VK_SPACE", VK_SPACE);       // Space
    
    // Windows specific keys from config
    map.insert("VK_LWIN", VK_LWIN);         // Left Windows key
    map.insert("VK_RWIN", VK_RWIN);         // Right Windows key
    map.insert("VK_APPS", VK_APPS);         // Application key (menu key)
    map.insert("VK_SNAPSHOT", VK_SNAPSHOT); // Print Screen key
    map.insert("VK_SLEEP", VK_SLEEP);       // Sleep key

    // ... etc.
    return map;
}

DWORD SystemInteractionModule::stringToVkCode(const QString& keyString) {
    return VK_CODE_MAP.value(keyString.toUpper(), 0); // Return 0 if not found
}

SystemInteractionModule::SystemInteractionModule(QObject *parent)
    : QObject{parent}
    , m_userModeActive(true) // Default to user mode active
    , m_isHookInstalled(false)
{
    instance_ = this; // Set the static instance pointer

    // Determine config path - consistent with AdminModule
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        qWarning() << "SystemInteractionModule: Could not get AppConfigLocation, falling back to applicationDirPath for config.";
        configDir = QCoreApplication::applicationDirPath();
    }
    QDir dir(configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "SystemInteractionModule: Could not create config directory:" << configDir << "Using application dir as last resort.";
            configDir = QCoreApplication::applicationDirPath(); // Fallback if creation fails
        }
    }
    m_configPath = configDir + "/config.json"; // Store for potential future use, though loadConfiguration also calculates it
    qDebug() << "SystemInteractionModule: Config path set to:" << m_configPath;

    if (!loadConfiguration()) {
        qWarning() << "系统交互模块(SystemInteractionModule): 配置文件加载失败，部分功能可能使用默认设置。";
        // Admin hotkey will be defaulted in loadConfiguration if needed
        // Blocked keys will be empty if not loaded, which is acceptable (no keys blocked by default)
    }
    // initializeBlockedKeys(); // Removed, logic moved to loadConfiguration
    qDebug() << "系统交互模块(SystemInteractionModule): 已创建。";
}

SystemInteractionModule::~SystemInteractionModule()
{
    uninstallKeyboardHook();
    
    // Stop all timers and delete MonitoringInfo objects before the map is cleared.
    // The QTimer objects themselves are parented to SystemInteractionModule 
    // and will be deleted by Qt's parent-child mechanism when SystemInteractionModule is deleted.
    for (auto it = m_monitoringApps.begin(); it != m_monitoringApps.end(); ++it) {
        MonitoringInfo* info = it.value();
        if (info) { // Check if pointer is valid
            if (info->timer) {
                info->timer->stop();
                // info->timer->deleteLater(); // QTimer is parented, Qt handles its deletion.
            }
            delete info; // Delete the MonitoringInfo object itself
        }
    }
    m_monitoringApps.clear(); // Clear the map of pointers

    if (instance_ == this) {
        instance_ = nullptr;
    }
    qDebug() << "[SystemInteractionModule] SystemInteractionModule destroyed.";
}

bool SystemInteractionModule::loadConfiguration() {
    m_adminLoginHotkey.clear(); // Clear previous hotkey before loading
    m_userModeBlockedVkCodes.clear(); // Clear previous blocked keys
    bool hotkeyLoaded = false;
    bool blockedKeysLoaded = false;

    // Determine config path - consistent with AdminModule
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        qWarning() << "SystemInteractionModule (loadConfiguration): Could not get AppConfigLocation, falling back to applicationDirPath for config.";
        configDir = QCoreApplication::applicationDirPath();
    }
    // Ensure directory exists (it should if AdminModule ran first, but good to be robust)
    QDir dirObj(configDir);
    if (!dirObj.exists()) {
        qWarning() << "SystemInteractionModule (loadConfiguration): Config directory" << configDir << "does not exist. Attempting to use application dir.";
        // It's unlikely this module should *create* the directory if it doesn't exist, 
        // as AdminModule usually handles initial config creation.
        // If AdminModule hasn't run or created it, SIM might operate with defaults.
        // For robustness if SIM is somehow initialized first or AdminModule fails to create dir:
        // configDir = QCoreApplication::applicationDirPath(); // Or just proceed and let file open fail for defaults
    }
    QString configPath = configDir + "/config.json";
    qDebug() << "SystemInteractionModule (loadConfiguration): Attempting to load config from:" << configPath;

    QFile configFile(configPath);

    if (!configFile.exists()) {
        qWarning() << "配置文件未找到:" << configPath << "将使用默认热键 (LCtrl+LShift+LAlt+L) 且不拦截用户模式按键。";
        m_adminLoginHotkey << VK_LCONTROL << VK_LSHIFT << VK_LMENU << 0x4C; // Default hotkey
        return false; // Indicate partial or failed load
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) { // QIODevice::Text is fine with QTextStream
        qWarning() << "无法打开配置文件:" << configPath << "将使用默认热键且不拦截用户模式按键。";
        m_adminLoginHotkey << VK_LCONTROL << VK_LSHIFT << VK_LMENU << 0x4C; // Default hotkey
        return false; // Indicate partial or failed load
    }

    QString configString;
    QTextStream in(&configFile);
    in.setEncoding(QStringConverter::Utf8); // Explicitly set UTF-8 encoding
    configString = in.readAll();
    configFile.close();

    if (configString.isEmpty()) {
        qWarning() << "配置文件为空或读取失败:" << configPath;
        m_adminLoginHotkey << VK_LCONTROL << VK_LSHIFT << VK_LMENU << 0x4C; // Default hotkey
        return false;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(configString.toUtf8(), &parseError); // Convert QString to UTF-8 QByteArray

    if (doc.isNull() || doc.isEmpty() || !doc.isObject()) {
        qWarning() << "解析配置文件失败:" << parseError.errorString() << "。错误详情: offset" << parseError.offset << "。将使用默认热键且不拦截用户模式按键。";
        m_adminLoginHotkey << VK_LCONTROL << VK_LSHIFT << VK_LMENU << 0x4C; // Default hotkey
        return false; // Indicate partial or failed load
    }

    QJsonObject rootObj = doc.object();
    // Load admin login hotkey
    if (rootObj.contains("shortcuts") && rootObj["shortcuts"].isObject()) {
        QJsonObject shortcutsObj = rootObj["shortcuts"].toObject();
        if (shortcutsObj.contains("admin_login") && shortcutsObj["admin_login"].isObject()) {
            QJsonObject adminLoginObj = shortcutsObj["admin_login"].toObject();
            if (adminLoginObj.contains("key_sequence") && adminLoginObj["key_sequence"].isArray()) {
                QJsonArray keySeqArray = adminLoginObj["key_sequence"].toArray();
                for (const QJsonValue& val : keySeqArray) {
                    if (val.isString()) {
                        DWORD vkCode = stringToVkCode(val.toString());
                        if (vkCode != 0) {
                            m_adminLoginHotkey.append(vkCode);
                        } else {
                            qWarning() << "配置文件中无效的管理员热键名:" << val.toString();
                        }
                    }
                }
            }
        }
    }

    if (!m_adminLoginHotkey.isEmpty()) {
        QStringList keyNames;
        for(DWORD code : m_adminLoginHotkey) { keyNames << VK_CODE_MAP.key(code, QString("0x%1").arg(code, 0, 16)); }
        qDebug() << "管理员登录热键已从配置文件加载:" << keyNames.join(" + ");
        hotkeyLoaded = true;
    } else {
        qWarning() << "未能在配置文件中找到有效的管理员登录热键配置，将使用默认热键。";
        m_adminLoginHotkey << VK_LCONTROL << VK_LSHIFT << VK_LMENU << 0x4C; // Default hotkey
    }

    // Load user mode blocked keys
    if (rootObj.contains("user_mode_settings") && rootObj["user_mode_settings"].isObject()) {
        QJsonObject userSettingsObj = rootObj["user_mode_settings"].toObject();
        if (userSettingsObj.contains("blocked_keys") && userSettingsObj["blocked_keys"].isArray()) {
            QJsonArray blockedKeysArray = userSettingsObj["blocked_keys"].toArray();
            for (const QJsonValue& val : blockedKeysArray) {
                if (val.isString()) {
                    DWORD vkCode = stringToVkCode(val.toString());
                    if (vkCode != 0) {
                        m_userModeBlockedVkCodes.insert(vkCode);
                    } else {
                        qWarning() << "配置文件中无效的用户模式拦截键名:" << val.toString();
                    }
                }
            }
            if (!m_userModeBlockedVkCodes.isEmpty()) {
                QStringList blockedKeyNames;
                for(DWORD code : m_userModeBlockedVkCodes) { blockedKeyNames << vkCodeToString(code); }
                qDebug() << "用户模式下需拦截的独立按键已从配置文件加载:" << blockedKeyNames.join(", ");
                blockedKeysLoaded = true;
            }
        } else {
            qDebug() << "配置文件中未找到 'user_mode_settings.blocked_keys' 或格式不正确。不拦截用户模式按键。";
        }
    } else {
        qDebug() << "配置文件中未找到 'user_mode_settings'。不拦截用户模式按键。";
    }

    return hotkeyLoaded; // Return true if at least admin hotkey was loaded successfully or defaulted.
                         // Blocked keys being empty is not a critical failure for loadConfiguration success status.
}

LRESULT CALLBACK SystemInteractionModule::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKbdStruct = (KBDLLHOOKSTRUCT *)lParam;
        DWORD vkCode = pKbdStruct->vkCode;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (instance_) {
            // Always update key state
            if (isKeyDown) {
                instance_->m_pressedKeys.insert(vkCode);
            } else if (isKeyUp) {
                instance_->m_pressedKeys.remove(vkCode);
            }

            // Admin login hotkey detection - moved out of userModeActive check
            // This hotkey should ideally always be available unless a more critical modal operation is in progress.
            if (isKeyDown && !instance_->m_adminLoginHotkey.isEmpty()) {
                bool allAdminHotkeyModifiersPressed = true;
                DWORD nonModifierAdminKey = 0; // Use DWORD, not int
                // QSet<DWORD> currentAdminComboPresses; // This set is not strictly needed for the logic to fire the signal

                for (DWORD requiredKey : instance_->m_adminLoginHotkey) {
                    bool isMod = instance_->isModifierKey(requiredKey);
                    
                    if (isMod) {
                        bool modifierFound = false;
                        if (requiredKey == VK_LCONTROL || requiredKey == VK_RCONTROL || requiredKey == VK_CONTROL) {
                            modifierFound = instance_->m_pressedKeys.contains(VK_LCONTROL) ||
                                            instance_->m_pressedKeys.contains(VK_RCONTROL) ||
                                            instance_->m_pressedKeys.contains(VK_CONTROL);
                        } else if (requiredKey == VK_LSHIFT || requiredKey == VK_RSHIFT || requiredKey == VK_SHIFT) {
                            modifierFound = instance_->m_pressedKeys.contains(VK_LSHIFT) ||
                                            instance_->m_pressedKeys.contains(VK_RSHIFT) ||
                                            instance_->m_pressedKeys.contains(VK_SHIFT);
                        } else if (requiredKey == VK_LMENU || requiredKey == VK_RMENU || requiredKey == VK_MENU) {
                            modifierFound = instance_->m_pressedKeys.contains(VK_LMENU) ||
                                            instance_->m_pressedKeys.contains(VK_RMENU) ||
                                            instance_->m_pressedKeys.contains(VK_MENU);
                        } else { 
                            // For any other key considered a modifier by isModifierKey (e.g. VK_LWIN)
                            modifierFound = instance_->m_pressedKeys.contains(requiredKey);
                        }

                        if (!modifierFound) {
                            allAdminHotkeyModifiersPressed = false;
                            break;
                        }
                        // currentAdminComboPresses.insert(requiredKey); // Not essential for triggering
                    } else { // It's a non-modifier key from the hotkey sequence
                        nonModifierAdminKey = requiredKey;
                        // We don't check vkCode against nonModifierAdminKey here yet.
                        // This loop is just to ascertain all *modifiers* are pressed and identify the nonModifierKey.
                    }
                }

                if (allAdminHotkeyModifiersPressed && nonModifierAdminKey != 0 && vkCode == nonModifierAdminKey) {
                    // All configured modifiers are pressed, and the current key press is the configured non-modifier key.
                    // Now, ensure ONLY the required keys for the admin hotkey are pressed.
                    bool allRequiredDown = true; // This will re-verify all keys, including the non-modifier.
                    for(DWORD reqKey : instance_->m_adminLoginHotkey) {
                        bool specificModFound = instance_->m_pressedKeys.contains(reqKey);
                        if (!specificModFound) {
                            // Handle generic in config, specific key pressed by user
                            if ((reqKey == VK_CONTROL && (instance_->m_pressedKeys.contains(VK_LCONTROL) || instance_->m_pressedKeys.contains(VK_RCONTROL))) ||
                                (reqKey == VK_SHIFT && (instance_->m_pressedKeys.contains(VK_LSHIFT) || instance_->m_pressedKeys.contains(VK_RSHIFT))) ||
                                (reqKey == VK_MENU && (instance_->m_pressedKeys.contains(VK_LMENU) || instance_->m_pressedKeys.contains(VK_RMENU)))) {
                                // generic in config, specific pressed - OK
                            } else {
                                allRequiredDown = false;
                                break;
                            }
                        }
                    }

                    if (allRequiredDown && instance_->m_pressedKeys.size() == instance_->m_adminLoginHotkey.size()) {
                        qDebug() << "系统交互模块(LowLevelKeyboardProc): 管理员登录热键组合被按下。";
                        QMetaObject::invokeMethod(instance_, "adminLoginRequested", Qt::QueuedConnection);
                        return 1; // Eat the key press that triggered the combo
                    }
                }
            }
            
            // User mode key blocking logic (only if user mode is active)
            if (instance_->m_userModeActive && isKeyDown) {
                qDebug() << QString("用户模式下按键: %1 (VK: 0x%2), m_userModeActive: %3")
                            .arg(instance_->vkCodeToString(vkCode))
                            .arg(vkCode, 2, 16, QChar('0'))
                            .arg(instance_->m_userModeActive);

                // 1. Block individual keys
                if (instance_->m_userModeBlockedVkCodes.contains(vkCode)) {
                    // Before blocking, ensure this key is NOT part of a currently forming admin login hotkey
                    // This is a simple check: if the blocked key is in the admin hotkey, and other admin hotkey modifiers are also pressed,
                    // then don't block it yet, let the admin hotkey logic decide.
                    // This prevents blocking 'L' if 'Ctrl+Shift+Alt+L' is the admin hotkey.
                    bool partOfAdminHotkeyInProgress = false;
                    if (instance_->m_adminLoginHotkey.contains(vkCode)) {
                        int matchingAdminModifiers = 0;
                        for (DWORD adminKey : instance_->m_adminLoginHotkey) {
                            if (adminKey != vkCode && instance_->m_pressedKeys.contains(adminKey)) {
                                // Check if it's a modifier-like key
                                if (adminKey == VK_LCONTROL || adminKey == VK_RCONTROL || adminKey == VK_CONTROL || 
                                    adminKey == VK_LSHIFT  || adminKey == VK_RSHIFT  || adminKey == VK_SHIFT || 
                                    adminKey == VK_LMENU   || adminKey == VK_RMENU   || adminKey == VK_MENU) {
                                    matchingAdminModifiers++;
                                }
                            }
                        }
                        // If all *other* keys in adminLoginHotkey are modifiers and are pressed, consider it in progress
                        // This is a heuristic. A more robust way would be to check if all *modifier* components of the admin hotkey are pressed.
                        if (matchingAdminModifiers >= (instance_->m_adminLoginHotkey.count() -1) && instance_->m_adminLoginHotkey.count() > 1) {
                             partOfAdminHotkeyInProgress = true;
                        }
                    }

                    if (!partOfAdminHotkeyInProgress) {
                        qDebug() << "系统交互模块(LowLevelKeyboardProc): 用户模式下，拦截按键:" << vkCode;
                        return 1; // Block it
                    }
                }

                // 2. Block Alt+Tab
                bool altPressed = instance_->m_pressedKeys.contains(VK_LMENU) || instance_->m_pressedKeys.contains(VK_RMENU) || instance_->m_pressedKeys.contains(VK_MENU);
                if (altPressed && vkCode == VK_TAB) {
                     qDebug() << "系统交互模块(LowLevelKeyboardProc): 在用户模式下拦截 Alt+Tab.";
                     return 1; // Block Alt+Tab
                }

                // 3. Block Ctrl+Esc
                bool ctrlPressed = instance_->m_pressedKeys.contains(VK_LCONTROL) || instance_->m_pressedKeys.contains(VK_RCONTROL) || instance_->m_pressedKeys.contains(VK_CONTROL);
                if (ctrlPressed && vkCode == VK_ESCAPE) {
                     qDebug() << "系统交互模块(LowLevelKeyboardProc): 在用户模式下拦截 Ctrl+Esc.";
                     return 1; // Block Ctrl+Esc
                }
                
                qDebug() << QString("用户模式下按键 %1 未被任何规则拦截。").arg(instance_->vkCodeToString(vkCode));
            }
        }
    }
    return CallNextHookEx(keyboardHook_, nCode, wParam, lParam);
}

bool SystemInteractionModule::installKeyboardHook()
{
    if (keyboardHook_ != NULL) {
        qDebug() << "键盘钩子: 已安装，无需重复安装。";
        return true; // Or false, depending on desired behavior for re-installation
    }

    // Get the module handle for the current instance
    // For a DLL, this would be GetModuleHandle(TEXT("MyHookDLL.dll"));
    // For an EXE, GetModuleHandle(NULL) gets the handle of the EXE itself.
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance) {
        qWarning() << "键盘钩子: 获取模块句柄失败，错误代码:" << GetLastError();
        return false;
    }

    keyboardHook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,         // Hook type: Low-level keyboard input events
        LowLevelKeyboardProc,   // Pointer to the hook procedure
        hInstance,              // Handle to the DLL containing the hook procedure (or NULL for thread-specific hooks if lpfn is in current process)
                                // For WH_KEYBOARD_LL, hInstance is the handle to the module that contains the hook procedure.
        0                       // Thread ID: 0 means associate with all existing threads on the current desktop.
    );

    if (keyboardHook_ == NULL)
    {
        qWarning() << "键盘钩子: 安装失败，错误代码:" << GetLastError();
        return false;
    }

    qDebug() << "键盘钩子: 安装成功。";
    return true;
}

void SystemInteractionModule::uninstallKeyboardHook()
{
    if (keyboardHook_ != NULL)
    {
        if (UnhookWindowsHookEx(keyboardHook_))
        {
            qDebug() << "键盘钩子: 卸载成功。";
            keyboardHook_ = NULL;
        }
        else
        {
            qWarning() << "键盘钩子: 卸载失败，错误代码:" << GetLastError();
        }
    }
    else
    {
        qDebug() << "键盘钩子: 无需卸载，当前未安装钩子。";
    }
}

// Renaming and enhancing this function
void SystemInteractionModule::bringToFrontAndActivate(WId windowId)
{
    qDebug() << "系统交互模块(SystemInteractionModule): 请求置顶并激活窗口 WId:" << windowId;
    HWND hwnd = reinterpret_cast<HWND>(windowId);

    if (!IsWindow(hwnd)) {
        qWarning() << "系统交互模块(SystemInteractionModule): bringToFrontAndActivate - 无效的窗口句柄:" << windowId;
        return;
    }

    // Get the thread ID of the foreground window and our window
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    DWORD ourThreadId = GetWindowThreadProcessId(hwnd, nullptr);

    // Attach to the foreground window's thread input state if necessary
    if (foregroundThreadId != ourThreadId) {
        AttachThreadInput(foregroundThreadId, ourThreadId, TRUE);
    }

    // Standard bring to top and activate sequence
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    BringWindowToTop(hwnd); // Redundant with SetWindowPos HWND_TOPMOST but often used

    // Show the window if it's minimized
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    // Ensure it's not minimized
    ShowWindow(hwnd, SW_SHOWNA); // SW_SHOWNA shows it in its current state, if minimized SW_RESTORE is better.

    // Detach thread input state
    if (foregroundThreadId != ourThreadId) {
        AttachThreadInput(foregroundThreadId, ourThreadId, FALSE);
    }

    qDebug() << "系统交互模块(SystemInteractionModule): 尝试置顶并激活窗口句柄:" << hwnd;
}

// Callback function for EnumWindows (modified for hint-based search)
// CORRECTED: Added SystemInteractionModule:: scope
BOOL CALLBACK SystemInteractionModule::EnumWindowsProcWithHints(HWND hwnd, LPARAM lParam)
{
    HintedEnumWindowsCallbackArg* pArg = reinterpret_cast<HintedEnumWindowsCallbackArg*>(lParam);
    DWORD currentWindowProcessId;
    GetWindowThreadProcessId(hwnd, &currentWindowProcessId);

    if (currentWindowProcessId == pArg->targetPid) {
        // Basic visible check first
        if (!IsWindowVisible(hwnd)) {
            return TRUE; // Continue enumerating
        }

        // Check for cloaked windows (DWM)
        int cloaked = 0;
        DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        if (cloaked) {
            return TRUE; // Skip cloaked windows
        }

        wchar_t classNameStr[256];
        GetClassNameW(hwnd, classNameStr, 256);
        QString currentClassName = QString::fromWCharArray(classNameStr);

        wchar_t windowTitleStr[512];
        GetWindowTextW(hwnd, windowTitleStr, 512);
        QString currentTitle = QString::fromWCharArray(windowTitleStr);

        int currentScore = 0;
        bool possibleCandidate = true;

        // --- Hint-based Scoring --- 
        const QJsonObject& hints = *(pArg->hints);
        QString primaryClassNameHint = hints.value("primaryClassName").toString();
        QString titleContainsHint = hints.value("titleContains").toString();
        bool allowNonTopLevelHint = hints.value("allowNonTopLevel").toBool(false); // Default false
        int minScoreHint = hints.value("minScore").toInt(50); // Default 50 (was 60)
        // QStringList exStyleMustHave = hints.value("exStyleMustHave").toVariant().toStringList();
        // QStringList exStyleMustNotHave = hints.value("exStyleMustNotHave").toVariant().toStringList();

        // 1. Class Name Match (High Priority)
        if (!primaryClassNameHint.isEmpty() && currentClassName == primaryClassNameHint) {
            currentScore += 100;
        } else if (!primaryClassNameHint.isEmpty() && currentClassName.contains(primaryClassNameHint, Qt::CaseInsensitive)) {
            currentScore += 70; // Partial match (e.g. if hint is OpusApp, and class is OpusApp_123)
        } else if (!primaryClassNameHint.isEmpty()) {
            // If primaryClassNameHint is provided but doesn't match at all, heavily penalize or disqualify
            // For now, let's not disqualify, but it gets no points here.
        }

        // 2. Title Contains Match
        if (!titleContainsHint.isEmpty() && currentTitle.contains(titleContainsHint, Qt::CaseInsensitive)) {
            currentScore += 50;
        }

        // 3. Window Title Presence (General good sign)
        if (!currentTitle.isEmpty()) {
            currentScore += 20;
        } else {
            currentScore -= 10; // Penalize empty titles slightly unless class name is a strong match
        }

        // 4. Top-Level Status & Hint Compliance
        HWND parentHwnd = GetParent(hwnd);
        bool isEffectivelyTopLevel = (parentHwnd == nullptr || parentHwnd == GetDesktopWindow());
        
        if (isEffectivelyTopLevel) {
            currentScore += 30;
        } else if (allowNonTopLevelHint) {
            currentScore += 15; // Allowed non-top-level gets some points
        } else {
            // Is not top-level, and non-top-level is NOT allowed by hints
            possibleCandidate = false; // Disqualify
            currentScore = -1; // Ensure it's not picked
        }

        // 5. WS_EX_APPWINDOW Style (Appears in taskbar, usually a good sign for main windows)
        LONG exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_APPWINDOW) {
            currentScore += 40;
        }
        
        // todo: Could add exStyleMustHave / exStyleMustNotHave checks here from hints

        qDebug() << "    [EnumWindowsProcWithHints] HWND:" << hwnd << "PID:" << currentWindowProcessId
                 << "Class: '" << currentClassName << "' Title: '" << currentTitle.left(50) << "...'"
                 << "Visible:" << IsWindowVisible(hwnd) << "Top-Level:" << isEffectivelyTopLevel
                 << "Score:" << currentScore << "(Min Required:" << minScoreHint << ")";

        if (possibleCandidate && currentScore >= minScoreHint && currentScore > pArg->bestScore) {
            qDebug() << "        >>> [EnumWindowsProcWithHints New Best Candidate!] HWND:" << hwnd << "Score:" << currentScore 
                     << "(Prev Best:" << pArg->bestScore << ") Class:" << currentClassName << "Title:" << currentTitle.left(50);
            pArg->bestHwnd = hwnd;
            pArg->bestScore = currentScore;
            // If a very strong match (e.g., class and title), could consider returning FALSE to stop early.
            // For now, let it iterate all windows of the process to find the absolute best score.
        }
    }
    return TRUE; // Continue enumerating
}

// New findMainWindowForProcess that uses hints
// Modified to return pair of HWND and score
QPair<HWND, int> SystemInteractionModule::findMainWindowForProcessWithScore(DWORD processId, const QJsonObject& windowHints) {
    qDebug() << "[SystemInteractionModule] Attempting to find main window for PID:" << processId << "with hints:" << QJsonDocument(windowHints).toJson(QJsonDocument::Compact);
    HintedEnumWindowsCallbackArg callbackArg(processId, &windowHints);
    // Ensure the callback is properly scoped
    EnumWindows(SystemInteractionModule::EnumWindowsProcWithHints, reinterpret_cast<LPARAM>(&callbackArg));

    if (callbackArg.bestHwnd) {
        qDebug() << "[SystemInteractionModule] Best window found for PID" << processId << "is" << callbackArg.bestHwnd << "with score" << callbackArg.bestScore;
    } else {
        qDebug() << "[SystemInteractionModule] No suitable window found for PID" << processId << "with given hints and logic.";
    }
    return qMakePair(callbackArg.bestHwnd, callbackArg.bestScore);
}

// Original findMainWindowForProcess, now calls the new version and discards score for compatibility
HWND SystemInteractionModule::findMainWindowForProcess(DWORD processId, const QJsonObject& windowHints) {
    return findMainWindowForProcessWithScore(processId, windowHints).first;
}

HWND SystemInteractionModule::findMainWindowForProcessOrChildren(DWORD initialPid, const QString& executableNameHint) {
    qDebug() << "SystemInteractionModule: Attempting to find main window for PID:" << initialPid 
             << "or its children. Executable hint:" << executableNameHint;

    // 1. Try the initial PID directly
    HWND hwnd = findMainWindowForProcess(initialPid);
    if (hwnd) {
        qDebug() << "SystemInteractionModule: Found main window directly with initial PID:" << initialPid;
        return hwnd;
    }
    qDebug() << "SystemInteractionModule: Did not find main window with initial PID:" << initialPid << ". Searching children.";

    // 2. If not found, search for direct children of initialPid
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "SystemInteractionModule: CreateToolhelp32Snapshot failed. Error:" << GetLastError();
        return NULL;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HWND childHwnd = NULL;

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ParentProcessID == initialPid) {
                qDebug() << "SystemInteractionModule: Found child process. PID:" << pe32.th32ProcessID 
                         << "Name:" << QString::fromWCharArray(pe32.szExeFile) 
                         << "Parent PID:" << pe32.th32ParentProcessID;
                // Try to find main window for this child process
                hwnd = findMainWindowForProcess(pe32.th32ProcessID);
                if (hwnd) {
                    qDebug() << "SystemInteractionModule: Found main window for child PID:" << pe32.th32ProcessID;
                    childHwnd = hwnd; // Store it, but continue iterating in case there are multiple children
                                      // and a later one is a better candidate (though findMainWindowForProcess should pick best)
                                      // For now, take first found child's window.
                    break; // Found a window for a child, stop searching children
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    if (childHwnd) {
        return childHwnd;
    }

    // 3. Fallback or further strategies could be added here, e.g., searching by executableNameHint
    //    if initialPid process has exited and thus has no children linked to it anymore.
    //    For now, we only check direct children of an existing initialPid.

    qWarning() << "SystemInteractionModule: Could not find main window for PID:" << initialPid << "or any of its direct children.";
    return NULL;
}

void SystemInteractionModule::setUserModeActive(bool active)
{
    qDebug() << "系统交互模块(SystemInteractionModule): 设置用户模式状态为:" << active;
    m_userModeActive = active;
}

bool SystemInteractionModule::isUserModeActive() const {
    return m_userModeActive;
}

// Add implementations for isModifierKey and vkCodeToString
bool SystemInteractionModule::isModifierKey(DWORD vkCode) const {
    return (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL || vkCode == VK_CONTROL ||
            vkCode == VK_LSHIFT  || vkCode == VK_RSHIFT  || vkCode == VK_SHIFT ||
            vkCode == VK_LMENU   || vkCode == VK_RMENU   || vkCode == VK_MENU ||
            vkCode == VK_LWIN    || vkCode == VK_RWIN);
}

QString SystemInteractionModule::vkCodeToString(DWORD vkCode) const {
    // Iterate through the VK_CODE_MAP to find a matching QString key for the given DWORD value.
    // This ensures that the returned string is one that stringToVkCode can parse back.
    for (auto it = VK_CODE_MAP.constBegin(); it != VK_CODE_MAP.constEnd(); ++it) {
        if (it.value() == vkCode) {
            return it.key(); // Return the canonical string representation from the map
        }
    }

    // Fallback for F-keys if they were somehow not in the map or if the map is incomplete.
    // (initializeVkCodeMap should be comprehensive for F1-F12).
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return QString("F%1").arg(vkCode - VK_F1 + 1);
    }
    
    // If no string representation is found in the map, return the hex code as a last resort.
    qWarning() << "SystemInteractionModule::vkCodeToString - No string representation found in VK_CODE_MAP for vkCode: 0x" + QString::number(vkCode, 16).toUpper();
    return QString("0x%1").arg(vkCode, 2, 16, QChar('0')).toUpper();
}

QStringList SystemInteractionModule::getCurrentAdminLoginHotkeyStrings() const
{
    QStringList keyNames;
    for (DWORD code : m_adminLoginHotkey) {
        keyNames << vkCodeToString(code); // Use existing vkCodeToString for consistency
    }
    return keyNames;
}

QIcon SystemInteractionModule::getIconForExecutable(const QString& executablePath)
{
    qDebug() << "[SIM::getIcon] Attempting to get icon for:" << executablePath;
    if (executablePath.isEmpty() || !QFile::exists(executablePath)) {
        qWarning() << "[SIM::getIcon] Path is empty or file does not exist:" << executablePath;
        return QIcon();
    }

    // --- Attempt 1: Using QFileIconProvider ---
    QFileIconProvider provider;
    QFileInfo fileInfo(executablePath);
    QIcon icon = provider.icon(fileInfo);
    qDebug() << "[SIM::getIcon] QFileIconProvider.icon(fileInfo) called.";

    if (!icon.isNull()) {
        qDebug() << "[SIM::getIcon] QFileIconProvider returned a non-null icon.";
        if (!icon.availableSizes().isEmpty()) {
            QPixmap pix = icon.pixmap(icon.availableSizes().first());
            if (!pix.isNull()) {
                qDebug() << "[SIM::getIcon] Successfully got icon using QFileIconProvider for:" << executablePath << "Pixmap size:" << pix.size();
                return icon;
            }
            qDebug() << "[SIM::getIcon] QFileIconProvider icon.pixmap() was null.";
        } else {
            qDebug() << "[SIM::getIcon] QFileIconProvider icon has no availableSizes.";
        }
        qDebug() << "[SIM::getIcon] QFileIconProvider returned an icon, but it seems to be a default/empty one for:" << executablePath;
    } else {
        qWarning() << "[SIM::getIcon] QFileIconProvider returned a null icon for:" << executablePath << "Trying SHGetFileInfoW.";
    }

    // --- Attempt 2: Using SHGetFileInfoW (with COM initialization) ---
    qDebug() << "[SIM::getIcon] Attempting SHGetFileInfoW for:" << executablePath;
    HRESULT comInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitializedHere = SUCCEEDED(comInitResult);
    if (!comInitializedHere && comInitResult != RPC_E_CHANGED_MODE) {
        qWarning() << "[SIM::getIcon] CoInitializeEx failed with HRESULT:" << QString::number(comInitResult, 16);
        // It might be okay to proceed if COM is already initialized in a compatible way.
    } else if (comInitializedHere) {
        qDebug() << "[SIM::getIcon] CoInitializeEx successful or already initialized in a compatible way.";
    }

    icon = QIcon(); // Reset icon before trying SHGetFileInfoW

    SHFILEINFOW sfi = {0};
    UINT flags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES; // Added SHGFI_USEFILEATTRIBUTES based on previous findings

    std::wstring filePathStdW = executablePath.toStdWString();
    const wchar_t* filePathW = filePathStdW.c_str();
    DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL; // Common attribute

    qDebug() << "[SIM::getIcon] Calling SHGetFileInfoW with flags:" << QString::number(flags, 16);
    if (SHGetFileInfoW(filePathW, fileAttributes, &sfi, sizeof(sfi), flags)) {
        qDebug() << "[SIM::getIcon] SHGetFileInfoW call succeeded.";
        if (sfi.hIcon) {
            qDebug() << "[SIM::getIcon] SHGetFileInfoW returned valid hIcon:" << sfi.hIcon;
            QImage image = QImage::fromHICON(sfi.hIcon);
            DestroyIcon(sfi.hIcon); // IMPORTANT: Destroy the icon handle

            if (!image.isNull()) {
                qDebug() << "[SIM::getIcon] QImage::fromHICON successful. Image size:" << image.size();
                QPixmap pixmap = QPixmap::fromImage(image);
                if (!pixmap.isNull()) {
                    icon = QIcon(pixmap);
                    qDebug() << "[SIM::getIcon] Successfully got icon using SHGetFileInfoW for:" << executablePath << "Pixmap size:" << pixmap.size();
                } else {
                    qWarning() << "[SIM::getIcon] SHGetFileInfoW: Failed to convert QImage to QPixmap for:" << executablePath;
                }
            } else {
                qWarning() << "[SIM::getIcon] SHGetFileInfoW: QImage::fromHICON failed for:" << executablePath;
            }
        } else {
            qWarning() << "[SIM::getIcon] SHGetFileInfoW succeeded but hIcon was null for:" << executablePath;
        }
    } else {
        DWORD error = GetLastError();
        qWarning() << "[SIM::getIcon] SHGetFileInfoW failed for:" << executablePath << "Error code:" << error;
    }

    if (comInitializedHere && comInitResult != RPC_E_CHANGED_MODE) {
         CoUninitialize();
         qDebug() << "[SIM::getIcon] CoUninitialize called.";
    }

    if(icon.isNull()){
        qWarning() << "[SIM::getIcon] All attempts to get icon FAILED for:" << executablePath;
    }
    return icon;
}

DWORD SystemInteractionModule::findProcessIdByName(const QString& executableName) {
    if (executableName.isEmpty()) {
        return 0; // Invalid argument
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "SystemInteractionModule::findProcessIdByName - CreateToolhelp32Snapshot failed. Error:" << GetLastError();
        return 0;
    }

    PROCESSENTRY32W pe32; // Use PROCESSENTRY32W for Unicode
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    DWORD foundPid = 0;

    std::wstring exeNameStdW = executableName.toStdWString();

    if (Process32FirstW(hSnapshot, &pe32)) { // Use Process32FirstW
        do {
            // Compare pe32.szExeFile with executableName (case-insensitive)
            QString currentProcessName = QString::fromWCharArray(pe32.szExeFile);
            if (currentProcessName.compare(executableName, Qt::CaseInsensitive) == 0) {
                foundPid = pe32.th32ProcessID;
                qDebug() << "SystemInteractionModule::findProcessIdByName - Found process:" << executableName << "with PID:" << foundPid;
                break; // Found the first match
            }
        } while (Process32NextW(hSnapshot, &pe32)); // Use Process32NextW
    }

    CloseHandle(hSnapshot);

    if (foundPid == 0) {
        // qDebug() << "SystemInteractionModule::findProcessIdByName - Process" << executableName << "not found.";
        // This log might be too verbose if called frequently during polling, enable if needed for debugging.
    }
    return foundPid;
}

// Constants for monitoring (can be defined globally in .cpp or as static const members)
const int MONITORING_INTERVAL_MS = 500; // milliseconds
const int MAX_MONITORING_ATTEMPTS = 40; // e.g., 40 * 500ms = 20 seconds. Increased from 20.
const int HINT_DETECTION_DELAY_MS = 5000; // 5 seconds delay for hint detection

void SystemInteractionModule::monitorAndActivateApplication(
    const QString& originalAppPath, 
    quint32 launcherPid, 
    const QString& mainExecutableHint, 
    const QJsonObject& windowHints, // New parameter
    bool forceActivateOnly)
{
    qDebug() << "SystemInteractionModule::monitorAndActivateApplication for" << originalAppPath 
             << "LauncherPID:" << launcherPid 
             << "MainExeHint:" << mainExecutableHint 
             << "ForceActivateOnly:" << forceActivateOnly
             << "WindowHints:" << QJsonDocument(windowHints).toJson(QJsonDocument::Compact);

    if (m_monitoringApps.contains(originalAppPath) && !forceActivateOnly) {
        qDebug() << "SystemInteractionModule: Already monitoring" << originalAppPath << "aborting new monitor request.";
        return;
    }

    QString targetExecutableName = mainExecutableHint;
    if (targetExecutableName.isEmpty()) {
        targetExecutableName = QFileInfo(originalAppPath).fileName();
        qDebug() << "SystemInteractionModule: mainExecutableHint is empty, using originalAppPath's filename as target:" << targetExecutableName;
    }

    // Ensure targetExecutableName ends with .exe
    if (!targetExecutableName.isEmpty() && !targetExecutableName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        qDebug() << "[SIM::monitorAndActivateApplication] Appending .exe to targetExecutableName:" << targetExecutableName;
        targetExecutableName.append(QStringLiteral(".exe"));
    }

    DWORD targetPid = findProcessIdByName(targetExecutableName);
    HWND hwnd = nullptr;

            if (targetPid != 0) {
        qDebug() << "SystemInteractionModule: Found target process" << targetExecutableName << "with PID:" << targetPid;
        bool useHints = !windowHints.isEmpty();
        if (useHints) {
            qDebug() << "SystemInteractionModule: Attempting to find main window for PID:" << targetPid << "(HINTED VERSION)";
            hwnd = findMainWindowForProcess(targetPid, windowHints);
        }
        
        if (!hwnd) {
            qDebug() << "SystemInteractionModule: Window not found with hints (or hints were empty). Trying generic findMainWindowForProcess for PID:" << targetPid;
            hwnd = findMainWindowForProcess(targetPid); // Fallback to non-hinted version
        }

        if (hwnd) {
            qDebug() << "SystemInteractionModule: Found main window" << hwnd << "for PID" << targetPid << (useHints ? "using hints." : "using generic search.");
            
            qDebug() << "SystemInteractionModule: Applying 500ms delay before activation.";
            QThread::msleep(500); 

            activateWindow(hwnd);
            qDebug() << "SystemInteractionModule: Activating window" << hwnd << "for" << originalAppPath;
                    emit applicationActivated(originalAppPath);

            auto it_remove_after_activate = m_monitoringApps.find(originalAppPath);
            if (it_remove_after_activate != m_monitoringApps.end()) {
                MonitoringInfo* infoToDel = it_remove_after_activate.value();
                if (infoToDel && infoToDel->timer) {
                    infoToDel->timer->stop(); 
                }
                m_monitoringApps.erase(it_remove_after_activate); 
                delete infoToDel; // Delete the MonitoringInfo object
                qDebug() << "SystemInteractionModule: Removed monitoring entry for" << originalAppPath << "after successful immediate activation.";
            }
        return;
                } else {
            qDebug() << "SystemInteractionModule: Found PID" << targetPid << "for" << targetExecutableName << "but failed to find its window even with fallbacks.";
        }
    }

        if (forceActivateOnly) {
        qDebug() << "SystemInteractionModule: Force activate only mode, but window not found immediately for" << targetExecutableName << "(PID:" << targetPid << "). Aborting.";
        emit applicationActivationFailed(originalAppPath, "Window not found in force activate mode");
        return;
    }

    qDebug() << "SystemInteractionModule: Target process/window for" << targetExecutableName << "not found immediately. Starting/Resetting monitoring timer for" << originalAppPath;
    
    if (m_monitoringApps.contains(originalAppPath)) {
        qDebug() << "SystemInteractionModule: Replacing existing monitoring info for" << originalAppPath;
        auto it_replace = m_monitoringApps.find(originalAppPath);
        if (it_replace != m_monitoringApps.end()) {
            MonitoringInfo* oldInfo = it_replace.value();
            if (oldInfo && oldInfo->timer) {
                oldInfo->timer->stop();
            }
            m_monitoringApps.erase(it_replace); 
            delete oldInfo; // Delete the old MonitoringInfo object
        }
    }

    MonitoringInfo* newMonitoringInfoRawPtr = new MonitoringInfo();
    newMonitoringInfoRawPtr->originalLauncherPath = originalAppPath;
    newMonitoringInfoRawPtr->mainExecutableHint = mainExecutableHint;
    newMonitoringInfoRawPtr->windowHintsJson = QJsonDocument(windowHints).toJson(QJsonDocument::Compact);
    newMonitoringInfoRawPtr->attempts = 0;
    newMonitoringInfoRawPtr->launcherPid = launcherPid;
    newMonitoringInfoRawPtr->forceActivateOnly = forceActivateOnly;
    newMonitoringInfoRawPtr->timer = new QTimer(this); 
    
    connect(newMonitoringInfoRawPtr->timer, &QTimer::timeout, this, &SystemInteractionModule::onMonitoringTimerTimeout);
    newMonitoringInfoRawPtr->timer->setProperty("originalAppPathProperty", originalAppPath); 
    newMonitoringInfoRawPtr->timer->start(1000); 
    
    m_monitoringApps[originalAppPath] = newMonitoringInfoRawPtr; // Store raw pointer
    
    qDebug() << "SystemInteractionModule: Monitoring timer started for" << originalAppPath << "to find" << targetExecutableName;
}

void SystemInteractionModule::onMonitoringTimerTimeout() {
    QTimer* firedTimer = qobject_cast<QTimer*>(sender());
    if (!firedTimer) return;

    QString originalAppPath = firedTimer->property("originalAppPathProperty").toString();

    if (originalAppPath.isEmpty() || !m_monitoringApps.contains(originalAppPath)) {
        qWarning() << "SystemInteractionModule::onMonitoringTimerTimeout - Timer fired for unknown or removed appPath:" << originalAppPath << "Timer object name:" << (firedTimer ? firedTimer->objectName() : "null");
        firedTimer->stop(); 
        // The timer is parented, so it will be deleted eventually with its parent (SystemInteractionModule).
        // If it was in m_monitoringApps, its unique_ptr would have been removed and MonitoringInfo deleted.
        // If it's not in m_monitoringApps, we can't remove its unique_ptr, but this indicates a logic flaw if a timer exists for an untracked app.
        return;
    }

    // Get a reference to the unique_ptr in the map.
    // We operate on the unique_ptr's managed object directly.
    MonitoringInfo* currentInfoPtr = m_monitoringApps.value(originalAppPath, nullptr); // Use .value() for safety

    if (!currentInfoPtr) { // Should not happen if map contains originalAppPath, but good check
        qWarning() << "SystemInteractionModule::onMonitoringTimerTimeout - currentInfoPtr is null for appPath:" << originalAppPath;
        firedTimer->stop();
        // Attempt to remove from map if it somehow still exists with a null pointer
        auto it_null_check = m_monitoringApps.find(originalAppPath);
        if (it_null_check != m_monitoringApps.end()) {
             m_monitoringApps.erase(it_null_check);
        }
                    return;
    }
    
    currentInfoPtr->attempts++;
    qDebug() << "SystemInteractionModule::onMonitoringTimerTimeout for" << originalAppPath << "Attempt:" << currentInfoPtr->attempts;

    QString targetExeName = currentInfoPtr->mainExecutableHint;
    if (targetExeName.isEmpty()) {
        targetExeName = QFileInfo(currentInfoPtr->originalLauncherPath).fileName();
    }
    // Ensure targetExeName ends with .exe for consistency during monitoring lookup
    if (!targetExeName.isEmpty() && !targetExeName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        qDebug() << "[SIM::onMonitoringTimerTimeout] Appending .exe to targetExeName:" << targetExeName;
        targetExeName.append(QStringLiteral(".exe"));
    }

    qDebug() << "[SIM::onMonitoringTimerTimeout] Attempting to find PID for:" << targetExeName;
    DWORD targetPid = findProcessIdByName(targetExeName);
    HWND targetHwnd = nullptr;

    if (targetPid != 0) {
        qDebug() << "[SIM::onMonitoringTimerTimeout] Found target process" << targetExeName << "with PID:" << targetPid;
        QJsonObject windowHints = QJsonDocument::fromJson(currentInfoPtr->windowHintsJson.toUtf8()).object();
        
        qDebug() << "[SIM::onMonitoringTimerTimeout] Calling findMainWindowForProcessWithScore with PID:" << targetPid << "and hints:" << currentInfoPtr->windowHintsJson;
        QPair<HWND, int> windowResult = findMainWindowForProcessWithScore(targetPid, windowHints);
        targetHwnd = windowResult.first;
        int score = windowResult.second;
        qDebug() << "[SIM::onMonitoringTimerTimeout] findMainWindowForProcessWithScore returned HWND:" << targetHwnd << "Score:" << score;
        
        if (!targetHwnd) { // If hints didn't find it, try generic (already part of findMainWindowForProcessWithScore logic, but explicit log is fine)
            qDebug() << "[SIM::onMonitoringTimerTimeout] Window not found with specific hints, trying generic findMainWindowForProcess for PID:" << targetPid;
            // The findMainWindowForProcessWithScore already incorporates a fallback if hints lead to no score or a low score
            // So, this explicit call might be redundant if findMainWindowForProcessWithScore's fallback is robust.
            // However, for logging clarity or if specific conditions are needed:
            // targetHwnd = findMainWindowForProcess(targetPid); 
            // qDebug() << "[SIM::onMonitoringTimerTimeout] Generic findMainWindowForProcess returned HWND:" << targetHwnd;
        }
        } else {
        qDebug() << "[SIM::onMonitoringTimerTimeout] Target process" << targetExeName << "not found (PID is 0).";
    }

    if (targetHwnd) {
        qDebug() << "SystemInteractionModule: Found window" << targetHwnd << "for" << targetExeName << "(PID:" << targetPid << ") during monitoring. Activating.";
        activateWindow(targetHwnd);
                    emit applicationActivated(originalAppPath);

        currentInfoPtr->timer->stop(); 
        auto it_success = m_monitoringApps.find(originalAppPath);
        if (it_success != m_monitoringApps.end()) {
            m_monitoringApps.erase(it_success); 
        }
        delete currentInfoPtr; // Delete the MonitoringInfo object
        qDebug() << "SystemInteractionModule: Monitoring successful for" << originalAppPath << ", entry removed.";
    } else if (currentInfoPtr->attempts >= MAX_MONITORING_ATTEMPTS) { 
        qWarning() << "SystemInteractionModule: Max monitoring attempts reached for" << originalAppPath << ". Could not find/activate" << targetExeName << ". Stopping timer.";
        emit applicationActivationFailed(originalAppPath, "Monitoring timeout"); 
        
        currentInfoPtr->timer->stop();
        auto it_fail = m_monitoringApps.find(originalAppPath);
        if (it_fail != m_monitoringApps.end()) {
            m_monitoringApps.erase(it_fail);
        }
        delete currentInfoPtr; // Delete the MonitoringInfo object
        qDebug() << "SystemInteractionModule: Monitoring failed for" << originalAppPath << "after" << MAX_MONITORING_ATTEMPTS << "attempts, entry removed.";
    }
    // If not found and attempts < max, timer continues automatically
}

void SystemInteractionModule::activateWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) { // Added IsWindow check
        qWarning() << "[SystemInteractionModule] activateWindow: Invalid or non-existent window handle:" << hwnd;
        return;
    }

    qDebug() << "[SystemInteractionModule] Attempting to activate window:" << hwnd;

    DWORD targetProcessId = 0;
    GetWindowThreadProcessId(hwnd, &targetProcessId); // Get PID of the target window

    // Call AllowSetForegroundWindow. ASFW_ANY is a broad permission.
    // If targetProcessId is valid, using it is more specific.
    if (targetProcessId != 0) {
        qDebug() << "[SystemInteractionModule] Calling AllowSetForegroundWindow(" << targetProcessId << ")";
        AllowSetForegroundWindow(targetProcessId);
    } else {
        qDebug() << "[SystemInteractionModule] Calling AllowSetForegroundWindow(ASFW_ANY) as target PID is 0.";
        AllowSetForegroundWindow(ASFW_ANY);
    }
    QThread::msleep(20); // Small delay after AllowSetForegroundWindow

    const int MAX_ACTIVATION_ATTEMPTS = 3;
    const int ACTIVATION_RETRY_DELAY_MS = 150; // Increased delay

    for (int attempt = 1; attempt <= MAX_ACTIVATION_ATTEMPTS; ++attempt) {
        qDebug() << "[SystemInteractionModule] Activation attempt:" << attempt << "for HWND:" << hwnd;

        // 1. Ensure it's not minimized and is visible
        if (IsIconic(hwnd)) {
            qDebug() << "[SystemInteractionModule] Window is iconic, restoring (SW_RESTORE).";
            ShowWindow(hwnd, SW_RESTORE);
            QThread::msleep(100); // Give it time to restore
        }
        // Ensure it's shown (SW_SHOWNA activates and displays it in its current size and position)
        // ShowWindow(hwnd, SW_SHOWNA); // This can sometimes be less effective than SW_RESTORE then SetForeground
        // QThread::msleep(50);

        // 2. Try to bring to top using SetWindowPos with HWND_TOPMOST
        // This makes the window the topmost window but doesn't necessarily give it focus.
        qDebug() << "[SystemInteractionModule] Calling SetWindowPos with HWND_TOPMOST.";
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        QThread::msleep(50); 

        // 3. Attempt to set as foreground window (this is the main function for activation)
        qDebug() << "[SystemInteractionModule] Calling SetForegroundWindow.";
        if (SetForegroundWindow(hwnd)) {
            qDebug() << "[SystemInteractionModule] SetForegroundWindow successful on attempt" << attempt;
    } else {
            DWORD error = GetLastError();
            qWarning() << "[SystemInteractionModule] SetForegroundWindow failed on attempt" << attempt << "GetLastError:" << error;
            
            // If SetForegroundWindow fails, try attaching thread input (common workaround)
            DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
            DWORD currentThreadId = GetCurrentThreadId(); // Our thread

            if (foregroundThreadId != 0 && currentThreadId != 0 && foregroundThreadId != currentThreadId) {
                qDebug() << "[SystemInteractionModule] Attaching thread input (current:" << currentThreadId << "fg:" << foregroundThreadId << ")";
                if (AttachThreadInput(foregroundThreadId, currentThreadId, TRUE)) {
                    qDebug() << "[SystemInteractionModule] AttachThreadInput successful. Retrying SetForegroundWindow.";
                    if (SetForegroundWindow(hwnd)) {
                         qDebug() << "[SystemInteractionModule] SetForegroundWindow successful after AttachThreadInput.";
        } else {
                         qWarning() << "[SystemInteractionModule] SetForegroundWindow STILL failed after AttachThreadInput. GetLastError:" << GetLastError();
            }
                    AttachThreadInput(foregroundThreadId, currentThreadId, FALSE);
                    qDebug() << "[SystemInteractionModule] Detached thread input.";
        } else {
                    qWarning() << "[SystemInteractionModule] AttachThreadInput failed. GetLastError:" << GetLastError();
                }
            }
        }
        QThread::msleep(50); // Delay after SetForegroundWindow attempts

        // 4. SetActiveWindow and SetFocus are generally less forceful for bringing to front but good for input focus
        qDebug() << "[SystemInteractionModule] Calling SetActiveWindow.";
        SetActiveWindow(hwnd); // Sets the active window
        qDebug() << "[SystemInteractionModule] Calling SetFocus.";
        SetFocus(hwnd);       // Sets keyboard focus
        
        // 5. BringWindowToTop is another function to ensure Z-order, often redundant but can help.
        qDebug() << "[SystemInteractionModule] Calling BringWindowToTop.";
        BringWindowToTop(hwnd);

        // 6. Check if our window is now the foreground window
        QThread::msleep(100); // Wait a bit for OS to process all these calls
        HWND currentForeground = GetForegroundWindow();
        qDebug() << "[SystemInteractionModule] Current foreground window after attempt" << attempt << "is:" << currentForeground << "(Target:" << hwnd << ")";

        if (currentForeground == hwnd) {
            qDebug() << "[SystemInteractionModule] Successfully set HWND:" << hwnd << "as foreground window on attempt" << attempt << ".";
            qDebug() << "[SystemInteractionModule] Window activation sequence completed for:" << hwnd;
            return; // Activation successful
        }

        qWarning() << "[SystemInteractionModule] Failed to set HWND:" << hwnd << "as foreground on attempt" << attempt
                   << ". Current foreground is:" << currentForeground << "(Last Error for SetForegroundWindow if it failed recently might not be accurate here).";

        if (attempt < MAX_ACTIVATION_ATTEMPTS) {
            qDebug() << "[SystemInteractionModule] Retrying activation in" << ACTIVATION_RETRY_DELAY_MS << "ms...";
            QThread::msleep(ACTIVATION_RETRY_DELAY_MS);
        }
    }

    qWarning() << "[SystemInteractionModule] Window activation sequence FAILED for HWND:" << hwnd << "after" << MAX_ACTIVATION_ATTEMPTS << "attempts.";
}

bool SystemInteractionModule::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    // For now, just return false, indicating the event was not handled by this filter.
    // Add specific event handling logic here if needed in the future.
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

// Make sure this function is defined before startExecutableDetection or called appropriately.
SuggestedWindowHints SystemInteractionModule::performExecutableDetectionLogic(const QString& executablePath, const QString& initialAppName) {
    qDebug() << "[SIM::performExeDetectLogic] Path:" << executablePath << "App Name:" << initialAppName;
    SuggestedWindowHints hints;
    hints.isValid = false;
    hints.detectedExecutableName = QFileInfo(executablePath).fileName(); // Initial exe name

    QProcess process;
    process.setProgram(executablePath);
    QDateTime launcherStartTime = QDateTime::currentDateTimeUtc(); // Record launcher start time
    process.start();

    DWORD initialPid = 0;
    if (!process.waitForStarted(15000)) { // Increased timeout
        qWarning() << "[SIM::performExeDetectLogic] Failed to start process:" << executablePath << "Error:" << process.errorString();
        hints.errorString = tr("无法启动目标程序: %1").arg(process.errorString());
        return hints;
    }
    initialPid = process.processId();
    qDebug() << "[SIM::performExeDetectLogic] Initial process started. PID:" << initialPid << "Exe:" << QFileInfo(executablePath).fileName();

    qDebug() << "[SIM::performExeDetectLogic] Waiting" << HINT_DETECTION_DELAY_MS << "ms for app to initialize...";
    QThread::msleep(HINT_DETECTION_DELAY_MS); // Wait for app to potentially launch its main window or child process

    QPair<HWND, int> windowResult = qMakePair(nullptr, -1); // HWND and score
    DWORD targetPid = initialPid; // Initially assume the launched process is the target
    
    bool initialProcessStillRunning = isProcessRunning(initialPid);
    qDebug() << "[SIM::performExeDetectLogic] Initial process PID" << initialPid << "still running:" << initialProcessStillRunning;
    
    QString actualDetectedExeName = QFileInfo(executablePath).fileName();

    if (initialProcessStillRunning) {
        qDebug() << "[SIM::performExeDetectLogic] Attempt 1: Finding window for initial PID:" << initialPid;
        // Pass an empty QJsonObject() if no specific hints are available for this call
        windowResult = findMainWindowForProcessWithScore(initialPid, QJsonObject()); 
        if (windowResult.first) {
            targetPid = initialPid; 
            actualDetectedExeName = getProcessNameByPid(targetPid);
            qDebug() << "[SIM::performExeDetectLogic] Window found for initial PID:" << initialPid << "HWND:" << windowResult.first << "Score:" << windowResult.second;
        }
    }

    if (!windowResult.first && !initialProcessStillRunning) {
        qDebug() << "[SIM::performExeDetectLogic] Initial process exited or no window found. Assuming launcher, searching for newer processes.";
        QList<DWORD> allPids = getAllProcessIds();
        QList<QPair<QDateTime, DWORD>> recentProcesses;

        for (DWORD pid : allPids) {
            if (pid == 0 || pid == initialPid) continue;
            QDateTime creationTime = getProcessCreationTime(pid);
            if (creationTime.isValid() && creationTime >= launcherStartTime.addMSecs(-2000) && creationTime <= QDateTime::currentDateTimeUtc().addMSecs(2000) ) {
                QString procName = getProcessNameByPid(pid);
                if (!procName.isEmpty() && 
                    !procName.contains("explorer.exe", Qt::CaseInsensitive) &&
                    !procName.contains("svchost.exe", Qt::CaseInsensitive) &&
                    !procName.contains("conhost.exe", Qt::CaseInsensitive) &&
                    !procName.contains("dwm.exe", Qt::CaseInsensitive) &&
                    !procName.contains("ctfmon.exe", Qt::CaseInsensitive) &&
                    !procName.contains("rundll32.exe", Qt::CaseInsensitive) &&
                    !procName.contains("ShellExperienceHost.exe", Qt::CaseInsensitive) &&
                    !procName.contains("StartMenuExperienceHost.exe", Qt::CaseInsensitive) &&
                    !procName.contains("SearchApp.exe", Qt::CaseInsensitive) &&
                    !procName.contains("TextInputHost.exe", Qt::CaseInsensitive) &&
                    !procName.contains(QFileInfo(executablePath).fileName(), Qt::CaseInsensitive)
                    ) {
                     recentProcesses.append({creationTime, pid});
                } else if (procName.contains(initialAppName, Qt::CaseInsensitive) && !initialAppName.isEmpty()){
                     recentProcesses.append({creationTime, pid});
                }
            }
        }
        std::sort(recentProcesses.begin(), recentProcesses.end(), [](const QPair<QDateTime, DWORD>& a, const QPair<QDateTime, DWORD>& b){
            return a.first > b.first;
        });
        qDebug() << "[SIM::performExeDetectLogic] Found" << recentProcesses.count() << "potential recent processes.";

        for (const auto& pair : recentProcesses) {
            DWORD potentialPid = pair.second;
            QString potentialExeName = getProcessNameByPid(potentialPid);
            qDebug() << "[SIM::performExeDetectLogic] Checking PID:" << potentialPid << "(" << potentialExeName << ")";
            // Pass an empty QJsonObject() if no specific hints are available for this call
            windowResult = findMainWindowForProcessWithScore(potentialPid, QJsonObject()); 
            if (windowResult.first) {
                targetPid = potentialPid;
                actualDetectedExeName = potentialExeName;
                qDebug() << "[SIM::performExeDetectLogic] Window found for recent PID:" << targetPid << "HWND:" << windowResult.first << "Exe:" << actualDetectedExeName << "Score:" << windowResult.second;
                break; 
            }
        }
    }

    if (windowResult.first) { // if HWND is not null
        hints.isValid = true;
        hints.processId = targetPid;
        hints.windowHandle = windowResult.first;
        // Ensure both detectedExecutableName and detectedMainExecutableName are set to the actual target exe
        hints.detectedExecutableName = actualDetectedExeName; 
        hints.detectedMainExecutableName = actualDetectedExeName; // <-- Explicitly set this
        hints.bestScoreDuringDetection = windowResult.second; // <<< STORE THE SCORE

        wchar_t className[256];
        GetClassName(hints.windowHandle, className, 256);
        hints.detectedClassName = QString::fromWCharArray(className);

        wchar_t windowTitle[256];
        GetWindowText(hints.windowHandle, windowTitle, 256);
        hints.exampleTitle = QString::fromWCharArray(windowTitle);

        LONG style = GetWindowLong(hints.windowHandle, GWL_STYLE);
        LONG exStyle = GetWindowLong(hints.windowHandle, GWL_EXSTYLE);

        hints.isTopLevel = (style & WS_CHILD) == 0; 
        hints.hasAppWindowStyle = (exStyle & WS_EX_APPWINDOW) != 0;
        
        qDebug() << "[SIM::performExeDetectLogic] Success! Detected Hints:" << hints.toString() << "Achieved Score:" << hints.bestScoreDuringDetection;

    } else {
        qWarning() << "[SIM::performExeDetectLogic] Could not find main window for:" << executablePath
                   << "(Initial PID:" << initialPid << ", Searched PID:" << targetPid << ")";
        hints.errorString = tr("未能找到 '%1' 的主窗口。").arg(initialAppName);
        hints.isValid = false;
    }

    if (initialPid != 0 && ( (initialPid != targetPid && isProcessRunning(initialPid)) || !windowResult.first) ) {
        qDebug() << "[SIM::performExeDetectLogic] Terminating initial process:" << QFileInfo(executablePath).fileName() << "(PID:" << initialPid << ")";
        process.kill(); 
        process.waitForFinished(2000); 
    }
    
    qDebug() << "[SIM::performExeDetectLogic] Finished. isValid:" << hints.isValid;
    return hints;
}

// This is the start of the existing startExecutableDetection function
void SystemInteractionModule::startExecutableDetection(const QString& executablePath, const QString& appName)
{
    qDebug() << "[SystemInteractionModule] Received request to detect executable parameters for:" << executablePath << "App Name:" << appName;

    // Use QtConcurrent::run to execute performExecutableDetectionLogic in a separate thread
    QFuture<void> future = QtConcurrent::run([this, executablePath, appName]() {
        qDebug() << "[SystemInteractionModule] Starting detection task in thread:" << QThread::currentThreadId();
        // Assuming performExecutableDetectionLogic is a member function
        // and it's thread-safe or primarily calls external processes/APIs.
        SuggestedWindowHints hints = this->performExecutableDetectionLogic(executablePath, appName);
        
        bool success = hints.isValid;
        QString errorString;
        if (!success) {
            // Construct a more meaningful error string if possible, 
            // or rely on performExecutableDetectionLogic to populate some part of hints even on failure.
            errorString = QString("Failed to detect main executable or window for '%1'.").arg(appName.isEmpty() ? executablePath : appName);
            qWarning() << "[SystemInteractionModule] Detection task failed for:" << executablePath << errorString;
        }

        qDebug() << "[SystemInteractionModule] Detection task finished for:" << executablePath << "Success:" << success;
        if (success) {
            qDebug() << "[SystemInteractionModule] Detected hints:" << hints.toString();
        }

        // Emit the signal. This will be marshalled to the main thread if the receiver lives there.
        emit detectionCompleted(hints, success, errorString);
    });
    // We don't explicitly wait for 'future' here to keep it non-blocking.
    // You can use QFutureWatcher if you need to monitor its progress/cancellation from the calling thread,
    // but for this task, emitting a signal upon completion is sufficient.
}

// इंप्लीमेंटेशन को SystemInteractionModule.cpp में जोड़ा जाना है।
void SystemInteractionModule::installHookAsync()
{
    qDebug() << "[SystemInteractionModule] installHookAsync() called.";
    if (installKeyboardHook()) {
        qDebug() << "[SystemInteractionModule] Keyboard hook installed asynchronously.";
            } else {
        qWarning() << "[SystemInteractionModule] Failed to install keyboard hook asynchronously.";
    }
}

void SystemInteractionModule::uninstallHookAsync()
{
    qDebug() << "[SystemInteractionModule] uninstallHookAsync() called.";
    uninstallKeyboardHook();
    qDebug() << "[SystemInteractionModule] Keyboard hook uninstalled asynchronously (or attempt was made).";
}

// Implementation for getAllProcessIds
QList<DWORD> SystemInteractionModule::getAllProcessIds() {
    QList<DWORD> pids;
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qWarning() << "[SIM::getAllProcessIds] CreateToolhelp32Snapshot failed. Error:" << GetLastError();
        return pids;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        qWarning() << "[SIM::getAllProcessIds] Process32First failed. Error:" << GetLastError();
        CloseHandle(hProcessSnap);
        return pids;
    }

    do {
        pids.append(pe32.th32ProcessID);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return pids;
}

// Implementation for getProcessCreationTime
QDateTime SystemInteractionModule::getProcessCreationTime(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) {
        // qWarning() << "[SIM::getProcessCreationTime] OpenProcess failed for PID" << processId << ". Error:" << GetLastError();
        return QDateTime(); // Return invalid QDateTime
    }

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        // qWarning() << "[SIM::getProcessCreationTime] GetProcessTimes failed for PID" << processId << ". Error:" << GetLastError();
        CloseHandle(hProcess);
        return QDateTime(); // Return invalid QDateTime
    }

    CloseHandle(hProcess);

    // Convert FILETIME to QDateTime
    ULARGE_INTEGER ull;
    ull.LowPart = ftCreation.dwLowDateTime;
    ull.HighPart = ftCreation.dwHighDateTime;
    
    // FILETIME is in 100-nanosecond intervals since January 1, 1601 (UTC).
    // QDateTime expects milliseconds since January 1, 1970 (UTC).
    // First, convert to seconds from epoch (1601-01-01).
    // Then, adjust for the difference between 1601 and 1970 epochs.
    // The number of 100-nanosecond intervals between 1601-01-01 and 1970-01-01 is 116444736000000000.
    qint64 fileTimeEpoch = static_cast<qint64>(ull.QuadPart);
    qint64 qtEpochDiff = 116444736000000000LL; // 100-nanosecond intervals
    
    if (fileTimeEpoch < qtEpochDiff) { // Should not happen for valid creation times
        return QDateTime();
    }

    qint64 msecsSince1970 = (fileTimeEpoch - qtEpochDiff) / 10000; // Convert 100-ns to ms

    return QDateTime::fromMSecsSinceEpoch(msecsSince1970, Qt::UTC);
}

// ADDED: Implementation for isProcessRunning
bool SystemInteractionModule::isProcessRunning(DWORD pid) {
    if (pid == 0) return false; // System Idle Process is not what we usually mean
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process != NULL) {
        DWORD exitCode;
        // GetExitCodeProcess returns TRUE if the process has not terminated
        // and also if it has terminated but the exit code is available.
        // If it returns TRUE and exitCode is STILL_ACTIVE, it's running.
        if (GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE) {
            CloseHandle(process);
            return true;
        }
        CloseHandle(process);
    }
    // If OpenProcess failed or exitCode is not STILL_ACTIVE
    return false;
}

// ADDED: Implementation for getProcessNameByPid
QString SystemInteractionModule::getProcessNameByPid(DWORD pid) {
    if (pid == 0) return QString();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "[SIM::getProcessNameByPid] CreateToolhelp32Snapshot failed. Error:" << GetLastError();
        return QString();
    }

    PROCESSENTRY32W pe32; // Use PROCESSENTRY32W for Unicode
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    QString processName;

    if (Process32FirstW(hSnapshot, &pe32)) { // Use Process32FirstW
        do {
            if (pe32.th32ProcessID == pid) {
                processName = QString::fromWCharArray(pe32.szExeFile);
                break; // Found the process
            }
        } while (Process32NextW(hSnapshot, &pe32)); // Use Process32NextW
    } else {
        qWarning() << "[SIM::getProcessNameByPid] Process32FirstW failed. Error:" << GetLastError();
    }

    CloseHandle(hSnapshot);
    if (processName.isEmpty()) {
       // qWarning() << "[SIM::getProcessNameByPid] Could not find process name for PID:" << pid;
    }
    return processName;
}

// Add the new function definition here
void SystemInteractionModule::stopMonitoringProcess(const QString& appPath) {
    qDebug() << "SystemInteractionModule::stopMonitoringProcess called for:" << appPath;
    if (m_monitoringApps.contains(appPath)) {
        MonitoringInfo* info = m_monitoringApps.take(appPath); // Remove from map and get the pointer
        if (info) {
            if (info->timer) {
                info->timer->stop();
                // Timer is assumed to be parented and auto-deleted, or handled by MonitoringInfo destructor if it owned it.
            }
            delete info; // Delete the MonitoringInfo struct itself
            qDebug() << "SystemInteractionModule: Stopped monitoring and cleaned up for" << appPath;
        }
    } else {
        qDebug() << "SystemInteractionModule: No active monitoring found for" << appPath << "to stop.";
    }
}

// ... (rest of the SystemInteractionModule.cpp file) ... 