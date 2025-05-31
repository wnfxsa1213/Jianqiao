#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QVariant>
#include <QTimer>
#include <QThread>
#include <QDateTime>
#include <QIcon>
#include <QPixmap>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QProcess>
#include <QCollator>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QWindow>
#include <QScreen>
#include <QImage>
#include <QSettings>
#include <QStyle>
#include <QApplication>
#include <QFuture>
#include <QtConcurrent>
// 系统和STL头文件
#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <qt_windows.h>
#include <VersionHelpers.h>
#include <string>
#include <map>
#include <algorithm>
// 自定义头文件
#include "SystemInteractionModule.h"
#include "AppStatus.h"
#include "common_types.h"


// 静态变量定义，必须在所有用到它的函数之前
static QList<WindowCandidateInfo> s_lastDetectionCandidates;
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

// 将HINT_DETECTION_DELAY_MS默认值提升到10000ms，并支持从配置读取
int getHintDetectionDelayMsFromConfig(const QString& configPath) {
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 10000; // 默认10秒
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return 10000;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("detection_wait_ms") && obj["detection_wait_ms"].isDouble()) {
        int val = obj["detection_wait_ms"].toInt();
        if (val >= 1000 && val <= 60000) return val; // 合理范围
    }
    return 10000;
}

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
    for (int i = 1; i <= 24; ++i) {
        map.insert(QString("F%1").arg(i), VK_F1 + (i - 1));
    }
    // 编辑/导航键
    map.insert("VK_DELETE", VK_DELETE);
    map.insert("VK_INSERT", VK_INSERT);
    map.insert("VK_HOME", VK_HOME);
    map.insert("VK_END", VK_END);
    map.insert("VK_PRIOR", VK_PRIOR);   // Page Up
    map.insert("VK_NEXT", VK_NEXT);     // Page Down
    map.insert("VK_BACK", VK_BACK);     // Backspace
    map.insert("VK_ESCAPE", VK_ESCAPE); // Esc
    map.insert("VK_RETURN", VK_RETURN); // Enter
    map.insert("VK_TAB", VK_TAB);       // Tab
    map.insert("VK_SPACE", VK_SPACE);   // Space
    // 方向键
    map.insert("VK_LEFT", VK_LEFT);
    map.insert("VK_UP", VK_UP);
    map.insert("VK_RIGHT", VK_RIGHT);
    map.insert("VK_DOWN", VK_DOWN);
    // 锁定键
    map.insert("VK_CAPITAL", VK_CAPITAL);   // Caps Lock
    map.insert("VK_NUMLOCK", VK_NUMLOCK);   // Num Lock
    map.insert("VK_SCROLL", VK_SCROLL);     // Scroll Lock
    // 小键盘
    map.insert("VK_NUMPAD0", VK_NUMPAD0);
    map.insert("VK_NUMPAD1", VK_NUMPAD1);
    map.insert("VK_NUMPAD2", VK_NUMPAD2);
    map.insert("VK_NUMPAD3", VK_NUMPAD3);
    map.insert("VK_NUMPAD4", VK_NUMPAD4);
    map.insert("VK_NUMPAD5", VK_NUMPAD5);
    map.insert("VK_NUMPAD6", VK_NUMPAD6);
    map.insert("VK_NUMPAD7", VK_NUMPAD7);
    map.insert("VK_NUMPAD8", VK_NUMPAD8);
    map.insert("VK_NUMPAD9", VK_NUMPAD9);
    map.insert("VK_MULTIPLY", VK_MULTIPLY);
    map.insert("VK_ADD", VK_ADD);
    map.insert("VK_SEPARATOR", VK_SEPARATOR);
    map.insert("VK_SUBTRACT", VK_SUBTRACT);
    map.insert("VK_DECIMAL", VK_DECIMAL);
    map.insert("VK_DIVIDE", VK_DIVIDE);
    // OEM 键（常见符号键）
    map.insert("VK_OEM_1", VK_OEM_1);
    map.insert("VK_OEM_PLUS", VK_OEM_PLUS);
    map.insert("VK_OEM_COMMA", VK_OEM_COMMA);
    map.insert("VK_OEM_MINUS", VK_OEM_MINUS);
    map.insert("VK_OEM_PERIOD", VK_OEM_PERIOD);
    map.insert("VK_OEM_2", VK_OEM_2);
    map.insert("VK_OEM_3", VK_OEM_3);
    map.insert("VK_OEM_4", VK_OEM_4);
    map.insert("VK_OEM_5", VK_OEM_5);
    map.insert("VK_OEM_6", VK_OEM_6);
    map.insert("VK_OEM_7", VK_OEM_7);
    // 其它常用
    map.insert("VK_PAUSE", VK_PAUSE);
    map.insert("VK_SNAPSHOT", VK_SNAPSHOT); // PrintScreen
    map.insert("VK_APPS", VK_APPS);         // 菜单键
    map.insert("VK_SLEEP", VK_SLEEP);
    // Windows specific keys from config
    map.insert("VK_LWIN", VK_LWIN);         // Left Windows key
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

// 新增：辅助函数，将VK Code列表转换为字符串，用于日志
QString SystemInteractionModule::vkCodesToString(const QList<DWORD>& codes) const {
    QStringList names;
    for (DWORD code : codes) {
        names.append(vkCodeToString(code)); // 复用现有的 vkCodeToString
    }
    if (names.isEmpty()) {
        return "[空组合]";
    }
    return names.join(" + ");
}

SystemInteractionModule::SystemInteractionModule(QObject *parent)
    : QObject{parent}
    , m_userModeActive(true) // Default to user mode active
    , m_isHookInstalled(false)
    , HINT_DETECTION_DELAY_MS(10000) // 初始化成员变量
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
    m_configPath = SystemInteractionModule::getConfigFilePath(); // Store for potential future use, though loadConfiguration also calculates it
    qDebug() << "SystemInteractionModule: Config path set to:" << m_configPath;

    if (!loadConfiguration()) {
        qWarning() << "系统交互模块(SystemInteractionModule): 配置文件加载失败，部分功能可能使用默认设置。";
        // Admin hotkey will be defaulted in loadConfiguration if needed
        // Blocked keys will be empty if not loaded, which is acceptable (no keys blocked by default)
    }
    // initializeBlockedKeys(); // Removed, logic moved to loadConfiguration
    qDebug() << "系统交互模块(SystemInteractionModule): 已创建。";

    // 加载等待时间
    HINT_DETECTION_DELAY_MS = getHintDetectionDelayMsFromConfig(m_configPath);
    qDebug() << "[SystemInteractionModule] 探测等待时间(ms):" << HINT_DETECTION_DELAY_MS;
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

    // 统一使用英文目录，避免中文路径
    QString configPath = SystemInteractionModule::getConfigFilePath();
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

        // 新增：加载用户模式下拦截的组合键
        if (userSettingsObj.contains("blocked_key_combinations") && userSettingsObj["blocked_key_combinations"].isArray()) {
            QJsonArray blockedCombosArray = userSettingsObj["blocked_key_combinations"].toArray();
            m_userModeBlockedKeyCombinations.clear(); // 清空旧的组合键配置
            for (const QJsonValue& comboVal : blockedCombosArray) {
                if (comboVal.isArray()) {
                    QJsonArray comboArray = comboVal.toArray();
                    QList<DWORD> currentComboVkCodes;
                    QStringList currentComboNames; // 用于日志
                    for (const QJsonValue& keyVal : comboArray) {
                        if (keyVal.isString()) {
                            DWORD vkCode = stringToVkCode(keyVal.toString());
                            if (vkCode != 0) {
                                currentComboVkCodes.append(vkCode);
                                currentComboNames.append(keyVal.toString());
                            } else {
                                qWarning() << "配置文件中无效的拦截组合键中的键名:" << keyVal.toString();
                            }
                        }
                    }
                    if (!currentComboVkCodes.isEmpty()) {
                        m_userModeBlockedKeyCombinations.append(currentComboVkCodes);
                        qDebug() << "用户模式下需拦截的组合键已加载:" << currentComboNames.join(" + ");
                    }
                }
            }
            if (!m_userModeBlockedKeyCombinations.isEmpty()) {
                // blockedKeysLoaded = true; // 可以考虑组合键是否也影响此标记，或者有单独的标记
            }
        } else {
            qDebug() << "配置文件中未找到 'user_mode_settings.blocked_key_combinations' 或格式不正确。不拦截组合键。";
        }

    } else {
        qDebug() << "配置文件中未找到 'user_mode_settings'。不拦截用户模式按键或组合键。";
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
                    qDebug() << QString("用户模式下拦截单个按键: %1 (VK: 0x%2)")
                                .arg(instance_->vkCodeToString(vkCode))
                                .arg(vkCode, 2, 16, QChar('0'));
                    return 1; // Eat the key press
                }

                // 2. 拦截组合键 (新增逻辑)
                if (!instance_->m_userModeBlockedKeyCombinations.isEmpty()) {
                    for (const QList<DWORD>& comboToBlock : instance_->m_userModeBlockedKeyCombinations) {
                        if (comboToBlock.isEmpty() || !comboToBlock.contains(vkCode)) {
                            // 如果组合是空的，或者当前按键不是这个组合的一部分，则跳过此组合
                            continue;
                        }

                        // 检查是否所有组合键都已按下，并且不多不少
                        bool allComboKeysCurrentlyPressed = true;
                        if (instance_->m_pressedKeys.size() != comboToBlock.size()) {
                            allComboKeysCurrentlyPressed = false;
                        } else {
                            for (DWORD requiredKeyInCombo : comboToBlock) {
                                bool keyFoundInPressed = instance_->m_pressedKeys.contains(requiredKeyInCombo);
                                
                                // 处理通用修饰符 (例如，配置为 VK_CONTROL，用户按下 VK_LCONTROL)
                                if (!keyFoundInPressed) {
                                    if (requiredKeyInCombo == VK_CONTROL && 
                                        (instance_->m_pressedKeys.contains(VK_LCONTROL) || instance_->m_pressedKeys.contains(VK_RCONTROL))) {
                                        keyFoundInPressed = true;
                                    } else if (requiredKeyInCombo == VK_SHIFT && 
                                               (instance_->m_pressedKeys.contains(VK_LSHIFT) || instance_->m_pressedKeys.contains(VK_RSHIFT))) {
                                        keyFoundInPressed = true;
                                    } else if (requiredKeyInCombo == VK_MENU && 
                                               (instance_->m_pressedKeys.contains(VK_LMENU) || instance_->m_pressedKeys.contains(VK_RMENU))) {
                                        keyFoundInPressed = true;
                                    }
                                }
                                if (!keyFoundInPressed) {
                                    allComboKeysCurrentlyPressed = false;
                                    break;
                                }
                            }
                        }

                        if (allComboKeysCurrentlyPressed) {
                            qDebug() << QString("用户模式下拦截组合键: %1 (触发键: %2 - 0x%3)")
                                        .arg(instance_->vkCodesToString(comboToBlock))
                                        .arg(instance_->vkCodeToString(vkCode))
                                        .arg(vkCode, 2, 16, QChar('0'));
                            return 1; // 吃掉消息，阻止组合键生效
                        }
                    }
                }
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
    qDebug() << "钩子安装，句柄:" << (quintptr)keyboardHook_;
    return true;
}

void SystemInteractionModule::uninstallKeyboardHook()
{
    qDebug() << "尝试卸载钩子，当前句柄:" << (quintptr)keyboardHook_;
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
        bool isVisible = IsWindowVisible(hwnd);
        bool isMinimized = IsIconic(hwnd);
        // 只要窗口可见或最小化，都进入后续打分
        if (!isVisible && !isMinimized) {
            return TRUE; // 既不可见也非最小化，跳过
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
        // primaryClassNameHint：主窗口类名Hint，若设置则优先匹配该类名（完全匹配加高分，部分匹配加中分）
        QString primaryClassNameHint = hints.value("primaryClassName").toString();
        // titleContainsHint：窗口标题包含的关键字Hint，若设置则窗口标题包含该关键字会加分
        QString titleContainsHint = hints.value("titleContains").toString();
        // allowNonTopLevelHint：是否允许非顶层窗口进入候选，默认false（如需兼容特殊窗口建议设为true）
        bool allowNonTopLevelHint = hints.value("allowNonTopLevel").toBool(true); // 默认true
        // minScoreHint：候选窗口最低分数线，低于此分数的窗口不会被选为主窗口，默认50
        int minScoreHint = hints.value("minScore").toInt(40); // 默认40（原为50）
        // exStyleMustHave / exStyleMustNotHave：可扩展的窗口扩展样式Hint，暂未启用
        // QStringList exStyleMustHave = hints.value("exStyleMustHave").toVariant().toStringList();
        // QStringList exStyleMustNotHave = hints.value("exStyleMustNotHave").toVariant().toStringList();

        // 1. Class Name Match (High Priority)
        if (!primaryClassNameHint.isEmpty() && currentClassName == primaryClassNameHint) {
            currentScore += 100;
        } else if (!primaryClassNameHint.isEmpty() && currentClassName.contains(primaryClassNameHint, Qt::CaseInsensitive)) {
            currentScore += 70; // Partial match (e.g. if hint is OpusApp, and class is OpusApp_123)
        }
        // 优化：WPS专属逻辑
        if (currentClassName == "OpusApp" && currentTitle.contains("WPS Office")) {
            currentScore += 200; // 直接极高分，确保WPS窗口优先
        }
        else if (!primaryClassNameHint.isEmpty()) {
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
        // 新增：如果窗口是最小化状态，适当降低分数，但不直接排除
        if (isMinimized) {
            currentScore -= 20; // 最小化窗口降低分数，但不排除
            qDebug() << "[EnumWindowsProcWithHints] 注意：该窗口处于最小化状态，分数已降低。";
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

// ========== 递归查找主窗口与特殊类型支持 BEGIN ==========

/**
 * 递归查找指定进程及其所有子进程的主窗口，兼容模拟器、UWP、无边框等特殊类型。
 * @param processId 目标进程ID
 * @param windowHints 窗口查找Hint
 * @param depth 当前递归深度（默认0）
 * @param maxDepth 最大递归深度，防止死循环
 * @return 最优主窗口句柄及分数
 */
QPair<HWND, int> SystemInteractionModule::findMainWindowRecursive(
    DWORD processId, const QJsonObject& windowHints, int depth, int maxDepth) {
    if (depth > maxDepth) {
        qWarning() << "[递归主窗口查找] 超过最大递归深度，PID:" << processId << "，终止递归。";
        return qMakePair(nullptr, -1);
    }
    // 1. 先尝试本进程主窗口（始终用Hint打分）
    QPair<HWND, int> best = SystemInteractionModule::findMainWindowForProcessWithScore(processId, windowHints);
    if (best.first) {
        qDebug() << QString("[递归主窗口查找] 层级%1，PID:%2，找到主窗口:%3，分数:%4 (Hint生效)").arg(depth).arg(processId).arg((quintptr)best.first).arg(best.second);
        return best;
    }
    // 2. 枚举所有子进程，递归查找（递归时也传递Hint）
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qWarning() << "[递归主窗口查找] CreateToolhelp32Snapshot失败，PID:" << processId;
        return qMakePair(nullptr, -1);
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    QPair<HWND, int> bestChild = qMakePair(nullptr, -1);
    QList<DWORD> childPids;
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ParentProcessID == processId) {
                childPids.append(pe32.th32ProcessID);
                QPair<HWND, int> childResult = SystemInteractionModule::findMainWindowRecursive(pe32.th32ProcessID, windowHints, depth + 1, maxDepth);
                if (childResult.first && childResult.second > bestChild.second) {
                    bestChild = childResult;
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    if (bestChild.first) {
        qDebug() << QString("[递归主窗口查找] 层级%1，PID:%2，子进程找到主窗口:%3，分数:%4 (Hint生效)").arg(depth).arg(processId).arg((quintptr)bestChild.first).arg(bestChild.second);
        return bestChild;
    }
    // 3. 兜底策略：如果递归所有进程后依然没有找到主窗口，则全量枚举本进程及所有子进程的所有窗口，按标题长度和类名频率排序
    // 用于统计类名出现频率
    std::map<QString, int> classNameCount;
    // 用于记录所有候选窗口信息
    QList<WindowCandidateInfo> candidates;
    // 定义窗口枚举回调
    struct EnumData {
        DWORD targetPid;
        QList<WindowCandidateInfo>* pWindows;
    };
    EnumData data{processId, &candidates};
    auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lParam);
        DWORD winPid = 0;
        GetWindowThreadProcessId(hwnd, &winPid);
        if (winPid != d->targetPid) return TRUE;
        // 采集窗口类名
        wchar_t classNameBuf[256] = {0};
        GetClassNameW(hwnd, classNameBuf, 256);
        QString className = QString::fromWCharArray(classNameBuf);
        // 采集窗口标题
        wchar_t titleBuf[512] = {0};
        GetWindowTextW(hwnd, titleBuf, 512);
        QString title = QString::fromWCharArray(titleBuf);
        // 可见性
        bool isVisible = IsWindowVisible(hwnd);
        // 是否顶层窗口
        HWND parentHwnd = GetParent(hwnd);
        bool isTopLevel = (parentHwnd == nullptr || parentHwnd == GetDesktopWindow());
        // 只收集可见顶层且标题长度大于0的窗口
        if (isVisible && isTopLevel && !title.isEmpty() && title.length() > 0) {
            d->pWindows->append({hwnd, className, title, isVisible, isTopLevel, winPid, 0});
        }
        return TRUE;
    };
    // 枚举本进程所有窗口
    EnumWindows(enumProc, (LPARAM)&data);
    // 递归枚举所有子进程
    for (DWORD childPid : childPids) {
        EnumData childData{childPid, &candidates};
        EnumWindows(enumProc, (LPARAM)&childData);
    }
    // 统计类名频率，优先选择标题最长、类名出现频率最高的窗口
    QString mostFreqClassName;
    int maxCount = 0;
    for (const auto& c : candidates) {
        if (!c.className.isEmpty()) {
            classNameCount[c.className]++;
            if (classNameCount[c.className] > maxCount) {
                maxCount = classNameCount[c.className];
                mostFreqClassName = c.className;
            }
        }
    }
    // 先按标题长度降序，再按类名频率降序排序
    std::sort(candidates.begin(), candidates.end(), [&](const WindowCandidateInfo& a, const WindowCandidateInfo& b) {
        if (a.title.length() != b.title.length())
            return a.title.length() > b.title.length();
        return classNameCount[a.className] > classNameCount[b.className];
    });
    if (!candidates.isEmpty()) {
        qDebug() << "[兜底主窗口查找] 选择标题最长/类名高频窗口:" << candidates.first().className << candidates.first().title;
        return qMakePair(candidates.first().hwnd, 100); // 兜底分数100
    }
    // 若无候选，返回失败
    return qMakePair(nullptr, -1);
}

// ========== 递归查找主窗口与特殊类型支持 END ==========

// ... existing code ...
// 修改findMainWindowForProcessOrChildren，调用递归查找
HWND SystemInteractionModule::findMainWindowForProcessOrChildren(DWORD initialPid, const QString& executableNameHint) {
    qDebug() << "[增强] SystemInteractionModule: 递归查找主窗口，初始PID:" << initialPid << "，可执行名Hint:" << executableNameHint;
    QJsonObject emptyHints; // 可根据需要传递Hint
    QPair<HWND, int> result = findMainWindowRecursive(initialPid, emptyHints);
    if (result.first) {
        qDebug() << "[增强] SystemInteractionModule: 递归查找主窗口成功，HWND:" << (quintptr)result.first << "，分数:" << result.second;
        return result.first;
    }
    // 查找失败时，收集所有候选窗口并输出详细日志
    QList<WindowCandidateInfo> candidates;
    findMainWindowRecursiveWithCandidates(initialPid, emptyHints, candidates, 0, 4);
    qWarning() << "[增强] SystemInteractionModule: 递归查找主窗口失败，输出所有候选窗口信息：";
    for (const auto& c : candidates) {
        qWarning() << QString("HWND: %1, 类名: %2, 标题: %3, 可见: %4, 顶层: %5, PID: %6, 分数: %7")
                      .arg((quintptr)c.hwnd)
                      .arg(c.className)
                      .arg(c.title)
                      .arg(c.isVisible)
                      .arg(c.isTopLevel)
                      .arg(c.processId)
                      .arg(c.score);
    }
    return nullptr;
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

    // --- 优先尝试获取大尺寸图标 ---
    QFileIconProvider provider;
    QFileInfo fileInfo(executablePath);
    QIcon icon = provider.icon(fileInfo);
    qDebug() << "[SIM::getIcon] QFileIconProvider.icon(fileInfo) called.";

    QList<QSize> trySizes = {QSize(256,256), QSize(128,128), QSize(64,64), QSize(48,48), QSize(32,32), QSize(16,16)};
    QPixmap bestPixmap;
    QSize bestSize;
    for (const QSize& sz : trySizes) {
        QPixmap pix = icon.pixmap(sz);
        if (!pix.isNull() && pix.width() >= bestSize.width() && pix.height() >= bestSize.height()) {
            bestPixmap = pix;
            bestSize = pix.size();
            if (pix.width() >= 128 && pix.height() >= 128) break; // 已满足大尺寸
        }
    }
    if (!bestPixmap.isNull() && bestPixmap.width() >= 64 && bestPixmap.height() >= 64) {
        qDebug() << "[SIM::getIcon] Got large pixmap from QFileIconProvider, size:" << bestPixmap.size();
        return QIcon(bestPixmap);
    }

    // --- 若QFileIconProvider获取不到大图标，尝试SHGetFileInfo ---
    qDebug() << "[SIM::getIcon] Attempting SHGetFileInfoW for large icon:" << executablePath;
    HRESULT comInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInitializedHere = SUCCEEDED(comInitResult);
    if (!comInitializedHere && comInitResult != RPC_E_CHANGED_MODE) {
        qWarning() << "[SIM::getIcon] CoInitializeEx failed with HRESULT:" << QString::number(comInitResult, 16);
    }
    SHFILEINFOW sfi = {0};
    UINT flags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES;
    std::wstring filePathStdW = executablePath.toStdWString();
    const wchar_t* filePathW = filePathStdW.c_str();
    DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL;
    if (SHGetFileInfoW(filePathW, fileAttributes, &sfi, sizeof(sfi), flags)) {
        if (sfi.hIcon) {
            QImage image = QImage::fromHICON(sfi.hIcon);
            DestroyIcon(sfi.hIcon);
            if (!image.isNull() && (image.width() > bestSize.width() || image.height() > bestSize.height())) {
                QPixmap pixmap = QPixmap::fromImage(image);
                if (!pixmap.isNull()) {
                    bestPixmap = pixmap;
                    bestSize = pixmap.size();
                    qDebug() << "[SIM::getIcon] Got large icon from SHGetFileInfoW, size:" << bestPixmap.size();
                }
            }
        }
    }
    if (comInitializedHere && comInitResult != RPC_E_CHANGED_MODE) {
        CoUninitialize();
    }
    if (!bestPixmap.isNull() && bestPixmap.width() >= 64 && bestPixmap.height() >= 64) {
        return QIcon(bestPixmap);
    }

    // --- 若仍无大图标，尝试查找应用目录下ico/svg资源 ---
    QDir exeDir = QFileInfo(executablePath).absoluteDir();
    QString baseName = QFileInfo(executablePath).completeBaseName();
    QStringList iconCandidates = exeDir.entryList(QStringList{baseName+".ico", "app.ico", "icon.ico", baseName+".svg", "app.svg", "icon.svg"}, QDir::Files);
    for (const QString& iconFile : iconCandidates) {
        QString iconPath = exeDir.absoluteFilePath(iconFile);
        QIcon fileIcon(iconPath);
        QPixmap pix = fileIcon.pixmap(QSize(128,128));
        if (!pix.isNull() && (pix.width() > bestSize.width() || pix.height() > bestSize.height())) {
            bestPixmap = pix;
            bestSize = pix.size();
            qDebug() << "[SIM::getIcon] Got icon from app dir resource:" << iconPath << ", size:" << bestPixmap.size();
        }
    }
    if (!bestPixmap.isNull()) {
        return QIcon(bestPixmap);
    }
    qWarning() << "[SIM::getIcon] All attempts to get large icon FAILED for:" << executablePath;
    return icon; // 兜底返回QFileIconProvider原始icon
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
const int MAX_MONITORING_ATTEMPTS = 60; // 由原20提升到60，最长60秒
// 替换原有const int HINT_DETECTION_DELAY_MS = 5000;
int HINT_DETECTION_DELAY_MS = 10000;

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
            m_lastActivatedAppPath = originalAppPath; // 新增：记录最近一次被激活的应用
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

    // 在查找窗口前，针对 DroneVirtualFlight 特殊处理
    if (targetExecutableName.compare("DroneVirtualFlight.exe", Qt::CaseInsensitive) == 0) {
        // 优先查找 Shipping 进程
        DWORD shippingPid = findProcessIdByName("DroneVirtualFlight-Win64-Shipping.exe");
        if (shippingPid != 0) {
            qDebug() << "[特殊处理] 检测到虚幻引擎 Shipping 进程，优先查找其主窗口";
            hwnd = findMainWindowForProcess(shippingPid, windowHints);
            if (hwnd) {
                qDebug() << "[特殊处理] 成功找到 Shipping 进程主窗口，立即激活并降级主界面Z序";
                activateWindow(hwnd);
                // 激活后无需再进入后续监控定时器逻辑，直接返回
                m_lastActivatedAppPath = originalAppPath;
                emit applicationActivated(originalAppPath);
                return;
            } else {
                qDebug() << "[特殊处理] Shipping 进程存在但未找到主窗口，继续走常规流程";
            }
        } else {
            qDebug() << "[特殊处理] 未检测到 Shipping 进程，继续走常规流程";
        }
    }
}

void SystemInteractionModule::onMonitoringTimerTimeout() {
    QTimer* firedTimer = qobject_cast<QTimer*>(sender());
    if (!firedTimer) return;
    QString originalAppPath = firedTimer->property("originalAppPathProperty").toString();
    if (originalAppPath.isEmpty() || !m_monitoringApps.contains(originalAppPath)) {
        qWarning() << "SystemInteractionModule::onMonitoringTimerTimeout - Timer fired for unknown or removed appPath:" << originalAppPath << "Timer object name:" << (firedTimer ? firedTimer->objectName() : "null");
        firedTimer->stop(); 
        return;
    }
    MonitoringInfo* currentInfoPtr = m_monitoringApps.value(originalAppPath, nullptr);
    if (!currentInfoPtr) {
        qWarning() << "SystemInteractionModule::onMonitoringTimerTimeout - currentInfoPtr is null for appPath:" << originalAppPath;
        firedTimer->stop();
        auto it_null_check = m_monitoringApps.find(originalAppPath);
        if (it_null_check != m_monitoringApps.end()) {
             m_monitoringApps.erase(it_null_check);
        }
                    return;
    }
    currentInfoPtr->attempts++;
    qDebug() << "SystemInteractionModule::onMonitoringTimerTimeout for" << originalAppPath << "Attempt:" << currentInfoPtr->attempts;
    // 1. 全局遍历所有进程的所有窗口，按Hint优先级查找
        QJsonObject windowHints = QJsonDocument::fromJson(currentInfoPtr->windowHintsJson.toUtf8()).object();
    HWND foundHwnd = nullptr;
    int foundScore = -1;
    QList<DWORD> allPids = getAllProcessIds();
    for (DWORD pid : allPids) {
        QPair<HWND, int> result = findMainWindowForProcessWithScore(pid, windowHints);
        if (result.first && result.second > foundScore) {
            foundHwnd = result.first;
            foundScore = result.second;
        }
    }
    if (foundHwnd) {
        qDebug() << "SystemInteractionModule: 在全局窗口中找到匹配白名单Hint的窗口，HWND:" << foundHwnd << "Score:" << foundScore;
        activateWindow(foundHwnd);
        // 激活后统一调用主界面降级接口，确保外部窗口可见
        lowerMainWindowZOrder(3000);
        // ========== 新增：激活后如强力置顶开启，加入定时器监控 ==========
        if (m_forceTopmostEnabled && currentInfoPtr && foundHwnd) {
            currentInfoPtr->windowHandle = foundHwnd;
            // 定时器已在setForceTopmostEnabled中统一管理，这里只需保证windowHandle被记录
            qDebug() << "[置顶策略] 已将目标窗口加入强力置顶监控，HWND:" << foundHwnd;
        }
        emit applicationActivated(originalAppPath);
        currentInfoPtr->timer->stop(); 
        auto it_success = m_monitoringApps.find(originalAppPath);
        if (it_success != m_monitoringApps.end()) {
            m_monitoringApps.erase(it_success); 
        }
        delete currentInfoPtr;
        qDebug() << "SystemInteractionModule: Monitoring successful for" << originalAppPath << ", entry removed.";
        return;
    }
    // 超时后才彻底放弃
    if (currentInfoPtr->attempts >= MAX_MONITORING_ATTEMPTS) {
        qWarning() << "SystemInteractionModule: Max monitoring attempts reached for" << originalAppPath << ". Could not find/activate window. Stopping timer.";
        emit applicationActivationFailed(originalAppPath, "Monitoring timeout"); 
        currentInfoPtr->timer->stop();
        auto it_fail = m_monitoringApps.find(originalAppPath);
        if (it_fail != m_monitoringApps.end()) {
            m_monitoringApps.erase(it_fail);
        }
        delete currentInfoPtr;
        qDebug() << "SystemInteractionModule: Monitoring failed for" << originalAppPath << "after" << MAX_MONITORING_ATTEMPTS << "attempts, entry removed.";
    }
    // 未超时则继续监控
}

void SystemInteractionModule::activateWindow(HWND hwnd) {
    if (!hwnd) return;
    // 让主界面降级Z序，直到外部窗口失去焦点/关闭
    lowerMainWindowZOrderUntilExternalLost(hwnd);
    SetForegroundWindow(hwnd);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    qDebug() << "[窗口置顶] 已激活外部窗口并降级主界面Z序（持续检测外部窗口状态）";
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

    qDebug() << "[SIM::performExeDetectLogic] Waiting" << this->HINT_DETECTION_DELAY_MS << "ms for app to initialize...";
    QThread::msleep(this->HINT_DETECTION_DELAY_MS); // Wait for app to potentially launch its main window or child process

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
        
        // ====== 采集窗口与进程详细参数（新增） ======
        // 1. 进程完整路径
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPid);
        if (hProc) {
            wchar_t exePathBuf[MAX_PATH] = {0};
            if (GetModuleFileNameExW(hProc, NULL, exePathBuf, MAX_PATH)) {
                hints.processFullPath = QString::fromWCharArray(exePathBuf);
            }
            CloseHandle(hProc);
        }
        // 2. 父进程ID
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                do {
                    if (pe32.th32ProcessID == targetPid) {
                        hints.parentProcessId = pe32.th32ParentProcessID;
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
        // 3. 父窗口句柄
        hints.parentWindowHandle = GetParent(hints.windowHandle);
        // 4. 窗口层级
        int level = 0;
        HWND parent = hints.windowHandle;
        while ((parent = GetParent(parent)) != NULL) {
            ++level;
        }
        hints.windowHierarchyLevel = level;
        // 5. 是否可见
        hints.isVisible = IsWindowVisible(hints.windowHandle);
        // 6. 是否最小化
        hints.isMinimized = IsIconic(hints.windowHandle);
        // ====== 采集结束 ======
        
        qDebug() << "[SIM::performExeDetectLogic] Success! Detected Hints:" << hints.toString() << "Achieved Score:" << hints.bestScoreDuringDetection;

        } else {
        qWarning() << "[SIM::performExeDetectLogic] Could not find main window for:" << executablePath
                   << "(Initial PID:" << initialPid << ", Searched PID:" << targetPid << ")";
        hints.errorString = tr("未能找到 '%1' 的主窗口。").arg(initialAppName);
        hints.isValid = false;

        // 新增：收集所有候选窗口信息，便于UI展示
        QList<WindowCandidateInfo> candidates;
        findMainWindowRecursiveWithCandidates(initialPid, QJsonObject(), candidates, 0, 4);

        // 将候选窗口信息序列化为QJsonArray，存入hints.candidatesJson
        QJsonArray candidatesArray;
        for (const auto& c : candidates) {
            QJsonObject obj;
            obj["hwnd"] = QString::number((quintptr)c.hwnd);
            obj["className"] = c.className;
            obj["title"] = c.title;
            obj["isVisible"] = c.isVisible;
            obj["isTopLevel"] = c.isTopLevel;
            obj["processId"] = static_cast<int>(c.processId);
            obj["score"] = c.score;
            candidatesArray.append(obj);
        }
        hints.candidatesJson = candidatesArray; // 新增字段，供UI层读取
        // 新增：候选窗口为空时日志提示
        if (candidates.isEmpty()) {
            qWarning() << "[SIM::performExeDetectLogic] 未采集到任何候选窗口，建议检查进程是否正常启动或窗口是否被隐藏。";
        }
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

// 静态函数：统一获取配置文件路径，始终使用当前登录用户的USERPROFILE目录，避免管理员/普通用户路径不一致
QString SystemInteractionModule::getConfigFilePath() {
    // 通过环境变量USERPROFILE获取当前登录用户主目录，确保所有身份下配置一致
    QString userProfile = qEnvironmentVariable("USERPROFILE");
    QString configDir = userProfile + "/AppData/Roaming/雪鸮团队/剑鞘系统";
    QDir().mkpath(configDir); // 确保目录存在
    return configDir + "/config.json";
}

/**
 * @brief 获取所有白名单应用的实时状态
 * @param whitelist 当前白名单应用信息列表
 * @return 所有应用的AppStatus状态列表
 */
QList<AppStatus> SystemInteractionModule::getAllAppStatus(const QList<AppInfo>& whitelist) {
    QList<AppStatus> result;
    for (const AppInfo& info : whitelist) {
        AppStatus status;
        status.appName = info.name;
        status.exePath = info.exePath;
        status.icon = getIconForExecutable(info.exePath);
        status.pid = 0;
        status.hwnd = nullptr;
        status.status = AppRunStatus::NotRunning;
        status.lastActive = QDateTime();

        // 优先用mainExecutableHint查找进程，否则用path
        QString processName = !info.mainExecutableHint.isEmpty() ? info.mainExecutableHint : QFileInfo(info.exePath).fileName();
        DWORD pid = findProcessIdByName(processName);
        if (pid != 0) {
            status.pid = pid;
            // 查找主窗口
            HWND hwnd = findMainWindowForProcess(pid);
            status.hwnd = hwnd;
            if (hwnd) {
                // 判断窗口是否最小化、激活
                if (IsIconic(hwnd)) {
                    status.status = AppRunStatus::Minimized;
                } else if (GetForegroundWindow() == hwnd) {
                    status.status = AppRunStatus::Activated;
                } else {
                    status.status = AppRunStatus::Running;
                }
                status.lastActive = QDateTime::currentDateTime();
            } else {
                // 优化：进程存在但未找到主窗口，尝试查找所有属于该进程的窗口
                bool hasAnyWindow = false;
                EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                    DWORD winPid = 0;
                    GetWindowThreadProcessId(hwnd, &winPid);
                    if (winPid == (DWORD)lParam) {
                        // 只要有窗口就算
                        return FALSE; // 找到一个就停止
                    }
                    return TRUE;
                }, (LPARAM)pid);
                if (hasAnyWindow) {
                    status.status = AppRunStatus::Minimized; // 只要有窗口但未激活，视为最小化
                } else {
                    status.status = AppRunStatus::Running; // 无窗口，纯进程
                }
            }
        } else {
            status.status = AppRunStatus::NotRunning;
        }
        // TODO: 可扩展异常检测逻辑
        result.append(status);
        // 新增：如果该应用是最近一次被激活的应用，则强制高亮
        if (info.exePath == m_lastActivatedAppPath) {
            status.status = AppRunStatus::Activated;
        }
        // 新增：如果进程不存在且正好是上次激活的应用，清空记录
        if (info.exePath == m_lastActivatedAppPath) {
            m_lastActivatedAppPath.clear();
        }
    }
    return result;
}

/**
 * @brief 自动采集并推荐主窗口特征（windowFindingHints）
 * @param processId 目标进程ID
 * @return 推荐的windowFindingHints（包含primaryClassName、titleContains等）
 */
QJsonObject SystemInteractionModule::autoDetectWindowFindingHints(DWORD processId)
{
    // 用于统计类名出现频率
    std::map<QString, int> classNameCount;
    // 用于记录所有窗口标题
    QList<QString> windowTitles;
    // 用于记录所有窗口信息
    QList<WindowCandidateInfo> allWindows;

    // 定义窗口枚举回调
    struct EnumData {
        DWORD targetPid;
        QList<WindowCandidateInfo>* pWindows;
    };
    EnumData data{processId, &allWindows};

    auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lParam);
        DWORD winPid = 0;
        GetWindowThreadProcessId(hwnd, &winPid);
        if (winPid != d->targetPid) return TRUE;

        // 采集窗口类名
        wchar_t classNameBuf[256] = {0};
        GetClassNameW(hwnd, classNameBuf, 256);
        QString className = QString::fromWCharArray(classNameBuf);

        // 采集窗口标题
        wchar_t titleBuf[512] = {0};
        GetWindowTextW(hwnd, titleBuf, 512);
        QString title = QString::fromWCharArray(titleBuf);

        // 可见性
        bool isVisible = IsWindowVisible(hwnd);

        // 父窗口句柄
        HWND parentHwnd = GetParent(hwnd);

        // 是否顶层窗口
        bool isTopLevel = (parentHwnd == nullptr || parentHwnd == GetDesktopWindow());

        // 采集所有参数并加入列表
        d->pWindows->append({hwnd, className, title, isVisible, isTopLevel, winPid, 0});
        return TRUE;
    };
    // 枚举本进程所有窗口
    EnumWindows(enumProc, (LPARAM)&data);
    // 递归枚举所有子进程
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ParentProcessID == processId) {
                    EnumData childData{pe32.th32ProcessID, &allWindows};
                    EnumWindows(enumProc, (LPARAM)&childData);
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    // 统计类名频率和最长标题
    QString mostFreqClassName;
    int maxCount = 0;
    QString longestTitle;
    for (const auto& win : allWindows) {
        if (!win.className.isEmpty()) {
            classNameCount[win.className]++;
            if (classNameCount[win.className] > maxCount) {
                maxCount = classNameCount[win.className];
                mostFreqClassName = win.className;
            }
        }
        if (!win.title.isEmpty() && win.title.length() > longestTitle.length()) {
            longestTitle = win.title;
        }
    }
    // 构造推荐Hints
    QJsonObject hints;
    if (!mostFreqClassName.isEmpty())
        hints["primaryClassName"] = mostFreqClassName;
    if (!longestTitle.isEmpty())
        hints["titleContains"] = longestTitle;
    // 可根据需要添加更多特征
    return hints;
}

/**
 * @brief 递归查找主窗口并收集所有分数大于0的候选窗口
 * @param processId 目标进程ID
 * @param windowHints 查找Hint
 * @param candidates 用于收集所有候选窗口信息
 * @param depth 当前递归深度
 * @param maxDepth 最大递归深度
 * @return 最优主窗口句柄及分数
 */
QPair<HWND, int> SystemInteractionModule::findMainWindowRecursiveWithCandidates(
    DWORD processId,
    const QJsonObject& windowHints,
    QList<WindowCandidateInfo>& candidates,
    int depth,
    int maxDepth)
{
    if (depth > maxDepth) {
        qWarning() << "[递归主窗口查找] 超过最大递归深度，PID:" << processId << "，终止递归。";
        return qMakePair(nullptr, -1);
    }
    // 1. 本进程所有窗口打分，收集分数大于0的候选（始终用Hint打分）
    struct EnumData {
        DWORD targetPid;
        const QJsonObject* hints;
        QList<WindowCandidateInfo>* pCandidates;
        HWND bestHwnd = nullptr;
        int bestScore = -1;
    };
    EnumData data{processId, &windowHints, &candidates};
    auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lParam);
        DWORD winPid = 0;
        GetWindowThreadProcessId(hwnd, &winPid);
        if (winPid != d->targetPid) return TRUE;
        wchar_t classNameBuf[256] = {0};
        GetClassNameW(hwnd, classNameBuf, 256);
        QString className = QString::fromWCharArray(classNameBuf);
        wchar_t titleBuf[512] = {0};
        GetWindowTextW(hwnd, titleBuf, 512);
        QString title = QString::fromWCharArray(titleBuf);
        bool isVisible = IsWindowVisible(hwnd);
        HWND parentHwnd = GetParent(hwnd);
        bool isTopLevel = (parentHwnd == nullptr || parentHwnd == GetDesktopWindow());
        int score = 0;
        bool possibleCandidate = true;
        const QJsonObject& hints = *(d->hints);
        QString primaryClassNameHint = hints.value("primaryClassName").toString();
        QString titleContainsHint = hints.value("titleContains").toString();
        bool allowNonTopLevelHint = hints.value("allowNonTopLevel").toBool(false);
        int minScoreHint = hints.value("minScore").toInt(50);
        if (!primaryClassNameHint.isEmpty() && className == primaryClassNameHint) {
            score += 100;
        } else if (!primaryClassNameHint.isEmpty() && className.contains(primaryClassNameHint, Qt::CaseInsensitive)) {
            score += 70;
        }
        if (!titleContainsHint.isEmpty() && title.contains(titleContainsHint, Qt::CaseInsensitive)) {
            score += 50;
        }
        if (!title.isEmpty()) {
            score += 20;
        } else {
            score -= 10;
        }
        if (isTopLevel) {
            score += 30;
        } else {
            score += 10;
        }
        LONG exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_APPWINDOW) {
            score += 40;
        }
        if (IsIconic(hwnd)) {
            score -= 20;
        }
        // 只收集分数大于0的窗口
        if (possibleCandidate && score > 0) {
            d->pCandidates->append({hwnd, className, title, isVisible, isTopLevel, winPid, score});
        }
        // 记录最佳窗口
        if (possibleCandidate && score >= minScoreHint && score > d->bestScore) {
            d->bestHwnd = hwnd;
            d->bestScore = score;
        }
        // 日志输出每个窗口的Hint匹配和分数
        qDebug() << QString("[递归主窗口查找][Hint] PID:%1 HWND:%2 类名:%3 标题:%4 分数:%5").arg(winPid).arg((quintptr)hwnd).arg(className).arg(title.left(50)).arg(score);
        return TRUE;
    };
    EnumWindows(enumProc, (LPARAM)&data);
    // 2. 递归子进程（递归时也传递Hint）
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ParentProcessID == processId) {
                    findMainWindowRecursiveWithCandidates(pe32.th32ProcessID, windowHints, candidates, depth + 1, maxDepth);
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    s_lastDetectionCandidates = candidates;
    return qMakePair(data.bestHwnd, data.bestScore);
}

// ... existing code ...
// 新增成员变量保存最近一次候选窗口信息


QList<WindowCandidateInfo> SystemInteractionModule::getLastDetectionCandidates() const
{
    return s_lastDetectionCandidates;
}
// ... existing code ...

// ========== 置顶策略相关实现 ========== //
void SystemInteractionModule::setSmartTopmostEnabled(bool enabled) {
    m_smartTopmostEnabled = enabled;
    qDebug() << "[置顶策略] 智能置顶已" << (enabled ? "启用" : "禁用") << "。仅在检测到外部窗口被覆盖时再置顶，减少系统调用。";
}
void SystemInteractionModule::setForceTopmostEnabled(bool enabled) {
    m_forceTopmostEnabled = enabled;
    qDebug() << "[置顶策略] 强力置顶已" << (enabled ? "启用" : "禁用") << "。开启后每秒持续SetWindowPos置顶，防止被其他窗口抢占。";
    // 若启用强力置顶，启动定时器
    if (enabled) {
        if (!m_forceTopmostTimer) {
            m_forceTopmostTimer = new QTimer(this);
            connect(m_forceTopmostTimer, &QTimer::timeout, this, [this]() {
                // 遍历所有已激活的目标窗口，强制置顶
                for (auto it = m_monitoringApps.begin(); it != m_monitoringApps.end(); ++it) {
                    MonitoringInfo* info = it.value();
                    if (info && info->windowHandle) {
                        HWND hwnd = reinterpret_cast<HWND>(info->windowHandle);
                        if (IsWindow(hwnd)) {
                            // 每秒强制置顶一次，防止被其他窗口覆盖
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                        }
                    }
                }
            });
        }
        if (!m_forceTopmostTimer->isActive()) {
            m_forceTopmostTimer->start(1000); // 每秒强制置顶
        }
    } else {
        if (m_forceTopmostTimer && m_forceTopmostTimer->isActive()) {
            m_forceTopmostTimer->stop();
        }
    }
}
bool SystemInteractionModule::isSmartTopmostEnabled() const { return m_smartTopmostEnabled; }
bool SystemInteractionModule::isForceTopmostEnabled() const { return m_forceTopmostEnabled; }
// ... existing code ...
// 在monitorAndActivateApplication和onMonitoringTimerTimeout等激活窗口逻辑中，判断m_forceTopmostEnabled优先，若为true则持续定时置顶，否则按智能置顶逻辑（如检测被覆盖时再置顶）。
// ... existing code ...

// ... existing code ...
// ========== 主界面降级/恢复置顶实现 ========== //
/**
 * @brief 降低主界面Z序（临时取消置顶），延迟后自动恢复置顶
 * @param delayMs 降级持续时间（毫秒），默认3000ms
 * 调用场景：激活外部窗口后，主动降低主界面Z序，防止主界面抢占前台。
 */
void SystemInteractionModule::lowerMainWindowZOrder(int delayMs) {
    extern QWidget* mainWindow;
    if (mainWindow) {
        HWND mainHwnd = reinterpret_cast<HWND>(mainWindow->winId());
        SetWindowPos(mainHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        QTimer::singleShot(delayMs, [mainHwnd]() {
            SetWindowPos(mainHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        });
        qDebug() << "SystemInteractionModule: 主界面已降级Z序" << delayMs << "ms，确保外部窗口可见。";
    }
}
// ... existing code ...
// ========== 优化activateWindow，激活外部窗口后统一调用主界面降级 ========== //

// ... existing code ...
void SystemInteractionModule::lowerMainWindowZOrderUntilExternalLost(HWND externalHwnd) {
    extern QWidget* mainWindow;
    if (!mainWindow || !externalHwnd) return;
    HWND mainHwnd = reinterpret_cast<HWND>(mainWindow->winId());
    // 1. 先降级主界面Z序
    SetWindowPos(mainHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    m_lastActivatedExternalHwnd = externalHwnd;
    // 2. 启动定时器，持续检测外部窗口状态
    if (!m_topmostRestoreTimer) {
        m_topmostRestoreTimer = new QTimer(this);
        connect(m_topmostRestoreTimer, &QTimer::timeout, this, [this, mainHwnd]() {
            if (!m_lastActivatedExternalHwnd || !IsWindow(m_lastActivatedExternalHwnd)) {
                // 外部窗口已关闭
                SetWindowPos(mainHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                m_topmostRestoreTimer->stop();
                qDebug() << "[窗口置顶] 外部窗口已关闭，主界面恢复置顶";
                return;
            }
            // 检查外部窗口是否最小化或失去焦点
            if (IsIconic(m_lastActivatedExternalHwnd) || GetForegroundWindow() != m_lastActivatedExternalHwnd) {
                SetWindowPos(mainHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                m_topmostRestoreTimer->stop();
                qDebug() << "[窗口置顶] 外部窗口失去焦点或最小化，主界面恢复置顶";
            }
            // 否则继续保持主界面非置顶
        });
    }
    if (!m_topmostRestoreTimer->isActive()) {
        m_topmostRestoreTimer->start(500); // 每500ms检测一次
    }
    qDebug() << "[窗口置顶] 主界面降级Z序，持续检测外部窗口状态";
}
// ... existing code ...

// ... existing code ...
// ========== 宽松查找主窗口辅助函数 ========== //
/**
 * @brief 宽松查找：返回目标进程的第一个可见顶层窗口
 * @param processId 目标进程ID
 * @return HWND 第一个可见窗口句柄，找不到返回nullptr
 */
static DWORD s_looseFindProcessId = 0;
static HWND s_looseFindResultHwnd = nullptr;
static BOOL CALLBACK EnumWindowsProcForLooseFind(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    wchar_t className[256] = {0}, title[256] = {0};
    GetClassNameW(hwnd, className, 256);
    GetWindowTextW(hwnd, title, 256);
    BOOL isVisible = IsWindowVisible(hwnd);
    BOOL isMinimized = IsIconic(hwnd);
    // 调试日志
    qDebug() << "[窗口枚举] HWND:" << hwnd << "PID:" << pid
             << "Class:" << QString::fromWCharArray(className)
             << "Title:" << QString::fromWCharArray(title)
             << "Visible:" << isVisible << "Minimized:" << isMinimized;
    if (pid == s_looseFindProcessId && isVisible && !isMinimized) {
        s_looseFindResultHwnd = hwnd;
        qDebug() << "[宽松兜底] 命中第一个可见窗口:" << hwnd;
        return FALSE;
    }
    return TRUE;
}
static HWND findFirstVisibleTopLevelWindow(DWORD processId) {
    s_looseFindProcessId = processId;
    s_looseFindResultHwnd = nullptr;
    EnumWindows(EnumWindowsProcForLooseFind, 0);
    return s_looseFindResultHwnd;
}
// ... existing code ...
// ========== 合并唯一findMainWindowForProcess实现 ========== //

