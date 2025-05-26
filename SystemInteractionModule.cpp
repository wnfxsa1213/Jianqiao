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

// Initialize static members
HHOOK SystemInteractionModule::keyboardHook_ = NULL;
SystemInteractionModule* SystemInteractionModule::instance_ = nullptr;
const QMap<QString, DWORD> SystemInteractionModule::VK_CODE_MAP = SystemInteractionModule::initializeVkCodeMap();

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
    if (instance_ == this) {
        instance_ = nullptr;
    }
    qDebug() << "系统交互模块: 已销毁。";
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

// Structure to pass data to EnumWindows callback
struct EnumWindowsCallbackArg {
    DWORD processId;
    HWND  hwndFound; // HWND of the main window
};

// Callback function for EnumWindows
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsCallbackArg *pArg = reinterpret_cast<EnumWindowsCallbackArg*>(lParam);
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == pArg->processId) {
        LONG exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        HWND parentHwnd = GetParent(hwnd);
        BOOL isVisible = IsWindowVisible(hwnd);
        int titleLen = GetWindowTextLength(hwnd);

        qDebug() << "  [EnumWindowsProc Debug] PID:" << processId << "HWnd:" << hwnd
                 << "Visible:" << isVisible << "Parent:" << parentHwnd
                 << "ExStyle:" << QString::asprintf("0x%lX", exStyle) << "TitleLen:" << titleLen;

        if (isVisible && parentHwnd == NULL) {
            // Prefer windows with a title
            if (titleLen > 0) {
                qDebug() << "    >>> [EnumWindowsProc Match!] Found Visible, NoParent, Titled HWnd:" << hwnd;
                pArg->hwndFound = hwnd;
                return FALSE; // Found a good candidate, stop.
            }
            // If no titled window found yet, take the first visible top-level one as a fallback
            if (pArg->hwndFound == NULL) {
                qDebug() << "    >>> [EnumWindowsProc Fallback Candidate] Found Visible, NoParent, NoTitle HWnd:" << hwnd;
                pArg->hwndFound = hwnd;
                // Continue searching in this iteration in case a titled one appears later in the Z-order
            }
        }
    }
    return TRUE; // Continue enumerating
}

// New Static callback for logging all windows (for debugging)
static BOOL CALLBACK LogAllWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    wchar_t windowTitle[256];
    GetWindowText(hwnd, windowTitle, 256);
    wchar_t className[256];
    GetClassName(hwnd, className, 256);

    qDebug() << "  [LogAllWindowsProc] HWND:" << hwnd 
             << "PID:" << processId 
             << "Title:" << QString::fromWCharArray(windowTitle)
             << "Class:" << QString::fromWCharArray(className)
             << "Visible:" << IsWindowVisible(hwnd)
             << "Parent:" << GetParent(hwnd);
    return TRUE; // Continue enumerating
}

HWND SystemInteractionModule::findMainWindowForProcess(DWORD processId)
{
    qDebug() << "SystemInteractionModule: Attempting to find main window for PID:" << processId << "(Initial PID to search)";
    
    // --- BEGIN TEMPORARY DEBUG LOGGING ---
    // qDebug() << "SystemInteractionModule: == Listing all top-level windows at start of findMainWindowForProcess (for PID:" << processId << ") ==";
    // EnumWindows(LogAllWindowsProc, NULL);
    // qDebug() << "SystemInteractionModule: == Finished listing all top-level windows ==";
    // --- END TEMPORARY DEBUG LOGGING ---

    EnumWindowsCallbackArg arg;
    arg.processId = processId;
    arg.hwndFound = NULL;

    for (int i = 0; i < 20; ++i) {
        // Reset hwndFound for each top-level pass of EnumWindows if we want the *last* best match from each pass
        // or, keep it to get the *first* best match across all passes.
        // Current logic: keeps the first titled one found, or the first non-titled if no titled ones appear.
        // If in one pass of EnumWindows multiple suitable windows exist, the one appearing earlier in Z-order that fits criteria will be chosen.
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&arg));
        if (arg.hwndFound != NULL) {
            // If a titled window was found, EnumWindowsProc would have returned FALSE and stopped.
            // If a non-titled one was found, EnumWindowsProc continued. We check here after the full pass.
            // Break here if we found *any* candidate in this pass to avoid unnecessary further sleeps/passes.
            // However, if a non-titled one was found, subsequent passes might find a titled one.
            // For simplicity, if anything is found, we log and break. The priority is in EnumWindowsProc.
            qDebug() << "SystemInteractionModule: Main window HWND:" << arg.hwndFound << "(candidate after attempt" << (i+1) << ") for PID:" << processId;
            break; 
        }
        Sleep(100); 
    }
    
    if (arg.hwndFound == NULL) {
        qWarning() << "SystemInteractionModule: Could not find any suitable main window for PID:" << processId << "after 20 attempts.";
    }
    return arg.hwndFound;
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
    if (executablePath.isEmpty() || !QFile::exists(executablePath)) {
        qWarning() << "SystemInteractionModule::getIconForExecutable - Path is empty or file does not exist:" << executablePath;
        return QIcon(); // Return an empty icon
    }

    SHFILEINFOW sfi = {0};
    UINT flags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES; // Added SHGFI_USEFILEATTRIBUTES

    // Convert QString to wchar_t* using std::wstring
    std::wstring filePathStdW = executablePath.toStdWString();
    const wchar_t* filePathW = filePathStdW.c_str();

    // Using SHGetFileInfoW for Unicode path support
    if (SHGetFileInfoW(filePathW, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), flags)) { // Pass FILE_ATTRIBUTE_NORMAL and use modified flags
        if (sfi.hIcon) {
            QImage image = QImage::fromHICON(sfi.hIcon);
            DestroyIcon(sfi.hIcon); // Important: Destroy the icon handle after conversion

            if (!image.isNull()) {
                QPixmap pixmap = QPixmap::fromImage(image);
                if (!pixmap.isNull()) {
                    return QIcon(pixmap);
                } else {
                     qWarning() << "SystemInteractionModule::getIconForExecutable - Failed to convert QImage to QPixmap for:" << executablePath;
                }
            } else {
                qWarning() << "SystemInteractionModule::getIconForExecutable - QImage::fromHICON failed for:" << executablePath;
            }
        } else {
            qWarning() << "SystemInteractionModule::getIconForExecutable - SHGetFileInfoW succeeded but hIcon was null for:" << executablePath;
        }
    } else {
        DWORD error = GetLastError();
        qWarning() << "SystemInteractionModule::getIconForExecutable - SHGetFileInfoW failed for:" << executablePath << "Error:" << error;
    }

    return QIcon(); // Return an empty icon if anything failed
} 