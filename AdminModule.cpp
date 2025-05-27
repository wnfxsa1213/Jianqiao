#include "AdminModule.h"
#include "AdminLoginView.h"
#include "AdminDashboardView.h"
#include "SystemInteractionModule.h"
#include "common_types.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QCryptographicHash>
#include <QTimer>

AdminModule::AdminModule(SystemInteractionModule* systemInteraction, AdminDashboardView* dashboardView, QObject *parent)
    : QObject(parent),
      m_loginView(new AdminLoginView(nullptr)), 
      m_adminDashboardView(dashboardView),
      m_systemInteractionModulePtr(systemInteraction),
      m_isAdminAuthenticated(false)
{
    qDebug() << "管理员模块(AdminModule): 构造函数开始。";

    loadConfig(); 

    connect(m_loginView, &AdminLoginView::loginAttempt, this, &AdminModule::onLoginAttempt);
    connect(m_loginView, &AdminLoginView::userRequestsExit, this, &AdminModule::onLoginViewRequestsExit);

    if (m_adminDashboardView) { 
        connect(m_adminDashboardView, &AdminDashboardView::whitelistChanged, this, &AdminModule::onWhitelistUpdated);
        connect(m_adminDashboardView, &AdminDashboardView::userRequestsExitAdminMode, this, &AdminModule::onUserRequestsExitAdminMode);
        connect(m_adminDashboardView, &AdminDashboardView::changePasswordRequested, this, &AdminModule::onChangePasswordRequested);
        connect(m_adminDashboardView, &AdminDashboardView::adminLoginHotkeyChanged, this, &AdminModule::onAdminLoginHotkeyChanged);
    }
    qDebug() << "管理员模块(AdminModule): 构造函数结束。";
}

AdminModule::~AdminModule()
{
    qDebug() << "管理员模块(AdminModule): 销毁。";
}

void AdminModule::showLoginView()
{
    qDebug() << "管理员模块(AdminModule): 请求显示管理员登录视图。";
    if (m_loginView) {
        m_loginView->resetUI();

        if (m_adminDashboardView && m_adminDashboardView->isVisible()) {
            qDebug() << "管理员模块(AdminModule): 管理员仪表盘视图可见，先隐藏它。";
            m_adminDashboardView->hide();
        }
        m_loginView->showNormal(); 
        m_loginView->activateWindow();
        m_loginView->raise();
        emit adminViewVisible(true);
        qDebug() << "管理员模块(AdminModule): 管理员登录视图已显示。";
    } else {
        qWarning() << "管理员模块(AdminModule): 登录视图实例为空，无法显示！";
    }
}

void AdminModule::requestExitAdminMode() {
    qDebug() << "管理员模块(AdminModule): 外部请求退出管理模式 (可能来自核心Shell)。";
    if (m_loginView && m_loginView->isVisible()) {
        m_loginView->hide();
    }
    // if (m_adminDashboardView && m_adminDashboardView->isVisible()) { // Visibility managed by StackedWidget
    //     m_adminDashboardView->hide();
    // }
    emit adminViewVisible(false); 
    emit exitAdminModeRequested(); 
}

void AdminModule::onUserRequestsExitAdminMode() {
    qDebug() << "管理员模块(AdminModule): onUserRequestsExitAdminMode 调用。";
    m_isAdminAuthenticated = false;
    // if (m_adminDashboardView && m_adminDashboardView->isVisible()) { // CoreShell handles this
    //     m_adminDashboardView->hide();
    // }
    // emit adminViewVisible(false); // CoreShell should manage this based on stack changes or specific signals
    emit exitAdminModeRequested(); 
}

void AdminModule::onLoginViewRequestsExit()
{
    qDebug() << "管理员模块(AdminModule): 登录视图请求退出管理员模式（例如通过取消按钮）。";
    if (m_loginView) {
        m_loginView->hide();
        onLoginViewHidden();
    }
    m_isAdminAuthenticated = false; 
    // if (!isAnyViewVisible()) { // This check might be complex with stacked widget
         // emit adminViewVisible(false); // Let CoreShell manage this
    emit exitAdminModeRequested(); // 通知 CoreShell 返回用户模式
    // }
}

void AdminModule::onLoginViewHidden() {
    qDebug() << "管理员模块(AdminModule): 登录视图已隐藏。";
    if (m_adminDashboardView && m_adminDashboardView->isVisible()) {
        // Whitelist manager is still open
    } else {
        qDebug() << "管理员模块(AdminModule): 登录视图隐藏，且管理员仪表盘视图也不可见。";
        if (!isAnyViewVisible()){ 
            emit adminViewVisible(false);
        }
    }
}

bool AdminModule::isAnyViewVisible() const {
    bool loginVisible = m_loginView && m_loginView->isVisible();
    bool dashboardVisible = m_adminDashboardView && m_adminDashboardView->isVisible();
    return loginVisible || dashboardVisible;
}

bool AdminModule::isLoginViewActive() const {
    return m_loginView && m_loginView->isVisible();
}

void AdminModule::onLoginAttempt(const QString& password)
{
    qDebug() << "管理员模块(AdminModule): 收到登录尝试。";
    if (verifyPassword(password)) {
        qDebug() << "管理员模块(AdminModule): 密码验证成功。";
        m_isAdminAuthenticated = true; 
        if(m_loginView) {
            m_loginView->notifyLoginResult(true); 
            // Hide login view as we are proceeding to dashboard (CoreShell will show dashboard)
            // It might be better for CoreShell to hide login view once login is confirmed.
            // For now, let login view handle its own hiding on success or AdminModule does it before emitting signal.
            if (m_loginView->isVisible()) {
                 m_loginView->hide(); 
                 onLoginViewHidden();
            }
        }
        emit loginSuccessfulAndAdminActive(); 
        // REMOVED: showAdminDashboardView(); // CoreShell will call prepareAdminDashboardData and switch view
    } else {
        qDebug() << "管理员模块(AdminModule): 密码验证失败。";
        m_isAdminAuthenticated = false; 
        if(m_loginView) {
            m_loginView->notifyLoginResult(false/*, "密码错误，请重试。"*/);
        }
    }
}

void AdminModule::onChangePasswordRequested(const QString& currentPassword, const QString& newPassword) {
    qDebug() << "管理员模块(AdminModule): 收到修改密码请求。";
    if (!verifyPassword(currentPassword)) {
        QMessageBox::warning(m_adminDashboardView, "密码错误", "当前管理员密码不正确。");
        qWarning() << "管理员模块(AdminModule): 修改密码失败 - 当前密码错误。";
        return;
    }
    if (saveAdminPasswordToConfig(newPassword)) {
        QMessageBox::information(m_adminDashboardView, "密码已更改", "管理员密码已成功更新。");
        qDebug() << "管理员模块(AdminModule): 管理员密码已成功更新。";
        emit configurationChanged(); 
    } else {
        QMessageBox::critical(m_adminDashboardView, "错误", "无法保存新密码。");
        qWarning() << "管理员模块(AdminModule): 保存新密码失败。";
    }
}

void AdminModule::onAdminLoginHotkeyChanged(const QList<DWORD>& newHotkeySequence) {
    qDebug() << "管理员模块(AdminModule): 收到新的管理员登录热键 (DWORD序列):" << newHotkeySequence;

    if (!m_systemInteractionModulePtr) {
        qWarning() << "管理员模块(AdminModule): SystemInteractionModule 指针为空，无法转换热键代码。";
        QMessageBox::critical(m_adminDashboardView, "热键保存失败", "内部错误，无法处理热键转换。");
        return;
    }

    QStringList newHotkeyVkStrings;
    for (DWORD code : newHotkeySequence) {
        QString strCode = m_systemInteractionModulePtr->vkCodeToString(code);
        if (strCode.isEmpty()) {
            qWarning() << "管理员模块(AdminModule): 无法将VK代码" << code << "转换为字符串表示。";
            // Decide if we should abort or continue with partial conversion
            // For now, let's try to append something or skip, but ideally, this shouldn't happen with valid codes
            newHotkeyVkStrings.append(QString("VK_%1").arg(code)); // Placeholder for unconvertable code
        } else {
            newHotkeyVkStrings.append(strCode);
        }
    }
    qDebug() << "管理员模块(AdminModule): 转换后的热键字符串:" << newHotkeyVkStrings.join(" + ");

    if (saveAdminLoginHotkeyToConfig(newHotkeyVkStrings)) {
        // m_systemInteractionModulePtr->loadConfiguration(); // This is already called inside saveAdminLoginHotkeyToConfig
        qDebug() << "管理员模块(AdminModule): 管理员登录热键已更新并保存到配置。 SystemInteractionModule 将重新加载配置。";
        emit configurationChanged();
    } else {
        qWarning() << "管理员模块(AdminModule): 保存新的管理员登录热键到配置失败。";
        QMessageBox::critical(m_adminDashboardView, "热键保存失败", "无法保存新的管理员登录热键。");
    }
}

void AdminModule::onWhitelistUpdated(const QList<AppInfo>& newWhitelist)
{
    qDebug() << "管理员模块(AdminModule): 白名单已更新，包含" << newWhitelist.count() << "个应用。";
    m_whitelistedApps.clear();

    for (const AppInfo& appFromView : newWhitelist) {
        AppInfo processedApp = appFromView; // Create a mutable copy
        qDebug() << "管理员模块(AdminModule): 处理来自视图的应用:" << processedApp.name << "路径:" << processedApp.path;
        
        // 自动检测逻辑已被移除。我们现在直接使用来自视图的 AppInfo。
        // 如果视图不直接提供 mainExecutableHint 和 windowFindingHints,
        // 那么这些字段将依赖于 config.json 中的手动配置。
        // 当保存时，如果这些字段在 appFromView 中是空的，它们也会被保存为空。
        // 用户需要在 config.json 中手动填写它们，或者如果 AdminDashboardView 支持，则在那里填写。

        // 只是为了确保从视图过来的 AppInfo 被正确记录
        qDebug() << "管理员模块(AdminModule): 应用 " << processedApp.name << " 的 mainExecutableHint: '" << processedApp.mainExecutableHint << "'";
        if (!processedApp.windowFindingHints.isEmpty()) {
            qDebug() << "  WindowFindingHints:" << QJsonDocument(processedApp.windowFindingHints).toJson(QJsonDocument::Compact);
        }

        m_whitelistedApps.append(processedApp);
    }

    if (saveWhitelistToConfig(m_whitelistedApps)) {
        qDebug() << "管理员模块(AdminModule): 更新后的白名单已成功保存到 config.json。";
        emit configurationChanged(); // 通知 UserModeModule 等其他模块配置已更改
    } else {
        qWarning() << "管理员模块(AdminModule): 更新白名单后保存到 config.json 失败!";
    }
}

void AdminModule::loadConfig()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.json";
    qDebug() << "管理员模块(AdminModule): 尝试从以下路径加载配置文件:" << configPath;

    QFile configFile(configPath);
    if (!configFile.exists()) {
        qWarning() << "管理员模块(AdminModule): 配置文件不存在，将创建默认配置。 Path:" << configPath;
        initializeDefaultAdminHotkey(); // Ensure hotkey is initialized before save
        saveConfig(); // Save a default config if it doesn't exist
        // After saving default, load it again to ensure consistency
        if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCritical() << "管理员模块(AdminModule): 无法读取新创建的默认配置文件！ Path:" << configPath;
            return; // Critical error, can't proceed
        }
    } else if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行读取。 Path:" << configPath;
        return;
    }

    QByteArray configData = configFile.readAll();
    configFile.close();
    QJsonDocument configDoc = QJsonDocument::fromJson(configData);

    if (configDoc.isNull() || !configDoc.isObject()) {
        qWarning() << "管理员模块(AdminModule): 配置文件格式无效 (不是JSON对象或解析失败)。 Path:" << configPath;
        // Consider attempting to save a default config here as well or alert user
        return;
    }

    QJsonObject rootObj = configDoc.object();

    // Load Admin Password Hash
    if (rootObj.contains("admin_password") && rootObj["admin_password"].isString()) {
        m_adminPasswordHash = rootObj["admin_password"].toString();
        qDebug() << "管理员模块(AdminModule): 已加载管理员密码哈希。";
    } else {
        qWarning() << "管理员模块(AdminModule): 配置文件中未找到或无效的管理员密码哈希，将设置默认密码。";
        saveAdminPasswordToConfig("123456"); // Sets m_adminPasswordHash and saves
    }

    // Load Admin Login Hotkey
    if (rootObj.contains("shortcuts") && rootObj["shortcuts"].isObject()) {
        QJsonObject shortcutsObj = rootObj["shortcuts"].toObject();
        if (shortcutsObj.contains("admin_login") && shortcutsObj["admin_login"].isObject()) {
            QJsonObject adminLoginObj = shortcutsObj["admin_login"].toObject();
            if (adminLoginObj.contains("key_sequence") && adminLoginObj["key_sequence"].isArray()) {
                QJsonArray hotkeyArray = adminLoginObj["key_sequence"].toArray();
                m_currentAdminLoginHotkeyVkCodes.clear();
                QStringList hotkeyStringsForLog;
                for (const QJsonValue& val : hotkeyArray) {
                    if (val.isString()) {
                         if(m_systemInteractionModulePtr){
                            DWORD vkCode = m_systemInteractionModulePtr->stringToVkCode(val.toString());
                            if (vkCode != 0) {
                                m_currentAdminLoginHotkeyVkCodes.append(vkCode);
                                hotkeyStringsForLog.append(val.toString());
                } else {
                                qWarning() << "管理员模块(AdminModule): 无法将热键字符串转换为VK代码:" << val.toString();
                }
            } else {
                             qWarning() << "管理员模块(AdminModule): SystemInteractionModule 未初始化，无法转换热键:" << val.toString();
            }
                    } else if (val.isDouble()) { // Legacy support for DWORDs directly stored as numbers
                        DWORD vkCode = static_cast<DWORD>(val.toDouble());
                         m_currentAdminLoginHotkeyVkCodes.append(vkCode);
                         if(m_systemInteractionModulePtr){
                            hotkeyStringsForLog.append(m_systemInteractionModulePtr->vkCodeToString(vkCode));
        } else {
                            hotkeyStringsForLog.append(QString("VK_%1").arg(vkCode));
                }
            }
                }
                qDebug() << "管理员模块(AdminModule): 已加载管理员登录热键:" << hotkeyStringsForLog.join(" + ");
            }
        }
    }
    if (m_currentAdminLoginHotkeyVkCodes.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 配置文件中未找到或无效的管理员登录热键，将设置默认热键。";
        initializeDefaultAdminHotkey(); // Sets m_currentAdminLoginHotkeyVkCodes
        // No need to save here as saveConfig() will be called if file was just created, or can be triggered by user action
    }

    // Load Whitelisted Apps
    m_whitelistedApps.clear();
    if (rootObj.contains("whitelist_apps") && rootObj["whitelist_apps"].isArray()) {
        QJsonArray appsArray = rootObj["whitelist_apps"].toArray();
        for (const QJsonValue &appVal : appsArray) {
            if (appVal.isObject()) {
                QJsonObject appObj = appVal.toObject();
                AppInfo appInfo;
                appInfo.name = appObj.value("name").toString();
                appInfo.path = appObj.value("path").toString();
                
                // 确保正确读取 mainExecutableHint 和 windowFindingHints
                appInfo.mainExecutableHint = appObj.value("mainExecutableHint").toString();
                appInfo.windowFindingHints = appObj.value("windowFindingHints").toObject();

                // Icon loading (priority: SystemInteractionModule, then direct path, then QIcon(appInfo.path))
            if (m_systemInteractionModulePtr) {
                    appInfo.icon = m_systemInteractionModulePtr->getIconForExecutable(appInfo.path);
                } else if (appObj.contains("icon_path") && appObj["icon_path"].isString()) {
                    // This is a fallback if SIM is not available, icon_path was a legacy idea
                    // For consistency, it's better to rely on SIM or QIcon(path)
                    appInfo.icon = QIcon(appObj["icon_path"].toString()); 
            }
                if (appInfo.icon.isNull() && !appInfo.path.isEmpty()) {
                     appInfo.icon = QIcon(appInfo.path); // Basic fallback if no icon found yet
                }

                if (!appInfo.name.isEmpty() && !appInfo.path.isEmpty()) {
                    m_whitelistedApps.append(appInfo);
                    qDebug() << "管理员模块(AdminModule): 已加载白名单应用:" << appInfo.name
                             << "Path:" << appInfo.path
                             << "Hint:" << appInfo.mainExecutableHint; // 确认Hint是否正确打印
                    if (!appInfo.windowFindingHints.isEmpty()) {
                         qDebug() << "  with WindowHints:" << QJsonDocument(appInfo.windowFindingHints).toJson(QJsonDocument::Compact);
                    }
                }
            }
        }
    }
    qDebug() << "管理员模块(AdminModule): 共加载" << m_whitelistedApps.size() << "个白名单应用。";

    // After loading all parts, if the dashboard view exists, populate it.
    if (m_adminDashboardView) {
        qDebug() << "管理员模块(AdminModule): 准备填充管理员仪表盘数据。";
        prepareAdminDashboardData(); 
    }

    qDebug() << "管理员模块(AdminModule): 配置文件加载完成。";
}

void AdminModule::saveConfig() 
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "管理员模块(AdminModule): 无法创建配置目录:" << configDir;
            return;
        }
    }
    QString configPath = configDir + "/config.json";
    qDebug() << "管理员模块(AdminModule): 准备保存完整配置到:" << configPath;

    QJsonObject rootObj;

    // Save Admin Password Hash (ensure it's loaded or default set before this)
    if (m_adminPasswordHash.isEmpty()) { 
        qWarning() << "管理员模块(AdminModule): 尝试保存配置时密码哈希为空，将设置并保存默认密码。";
        // This directly modifies m_adminPasswordHash AND writes only password to config
        // which might be overwritten by subsequent rootObj write. Better to just set member here.
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData("123456"); // Default password
        m_adminPasswordHash = QString::fromUtf8(hasher.result().toHex());
    }
    rootObj["admin_password"] = m_adminPasswordHash;

    // Save Admin Login Hotkey (ensure it's loaded or default set)
    if (m_currentAdminLoginHotkeyVkCodes.isEmpty()) {
         qWarning() << "管理员模块(AdminModule): 尝试保存配置时热键为空，将设置并保存默认热键。";
        initializeDefaultAdminHotkey(); // This sets m_currentAdminLoginHotkeyVkCodes
    }
    QJsonObject shortcutsObj;
    QJsonObject adminLoginObj;
    QJsonArray hotkeyArray;
    if (m_systemInteractionModulePtr) {
        for (DWORD vkCode : m_currentAdminLoginHotkeyVkCodes) {
            hotkeyArray.append(m_systemInteractionModulePtr->vkCodeToString(vkCode));
        }
    } else { // Fallback if SIM is null, save raw VK codes as strings (less ideal)
        for (DWORD vkCode : m_currentAdminLoginHotkeyVkCodes) {
            hotkeyArray.append(QString::number(vkCode)); // Or format as VK_XXX if possible without SIM
        }
         qWarning() << "管理员模块(AdminModule): SIM为空，热键将以数字形式保存。";
    }
    adminLoginObj["key_sequence"] = hotkeyArray;
    shortcutsObj["admin_login"] = adminLoginObj;
    rootObj["shortcuts"] = shortcutsObj;

    // Save Whitelisted Apps
        QJsonArray appsArray;
    for (const AppInfo &app : m_whitelistedApps) { // Use m_whitelistedApps member
            QJsonObject appObj;
            appObj["name"] = app.name;
            appObj["path"] = app.path;
        if (!app.mainExecutableHint.isEmpty()) {
            appObj["mainExecutableHint"] = app.mainExecutableHint;
        }
        if (!app.windowFindingHints.isEmpty()) {
            appObj["windowFindingHints"] = app.windowFindingHints;
        }
            appsArray.append(appObj);
        }
        rootObj["whitelist_apps"] = appsArray;

    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行写入:" << configPath;
        return;
    }
    QJsonDocument newDoc(rootObj);
    configFile.write(newDoc.toJson());
    configFile.close();
    qDebug() << "管理员模块(AdminModule): 完整配置已保存。";

    // After saving, SystemInteractionModule might need to reload its part of config (hotkeys)
    if (m_systemInteractionModulePtr) {
        qDebug() << "管理员模块(AdminModule): 通知SystemInteractionModule重新加载配置。";
        m_systemInteractionModulePtr->loadConfiguration(); // Ensure SIM reloads hotkeys
    }
}

void AdminModule::initializeDefaultAdminHotkey() {
    qDebug() << "管理员模块(AdminModule): 初始化默认管理员登录热键 (Ctrl+Alt+Shift+L)。";
    m_currentAdminLoginHotkeyVkCodes.clear();
    m_currentAdminLoginHotkeyVkCodes.append(VK_CONTROL);
    // m_currentAdminLoginHotkeyVkCodes.append(VK_MENU);    
    // m_currentAdminLoginHotkeyVkCodes.append(VK_SHIFT);
    // m_currentAdminLoginHotkeyVkCodes.append(0x4C); // L
    // initializeDefaultAdminHotkey 应该只设置内部变量，而不修改配置文件结构。
    // 配置文件的结构由 saveConfig 和 saveAdminLoginHotkeyToConfig 保证。
}

bool AdminModule::saveWhitelistToConfig(const QList<AppInfo>& apps)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "管理员模块(AdminModule): 无法创建配置目录:" << configDir;
            return false;
        }
    }
    QString configPath = configDir + "/config.json";
    qDebug() << "管理员模块(AdminModule): 准备将白名单保存到:" << configPath;

    QJsonObject rootObj;
    // Try to load existing config to preserve other settings
    QFile configFileRead(configPath);
    if (configFileRead.exists() && configFileRead.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray currentConfigData = configFileRead.readAll();
        configFileRead.close();
        QJsonDocument currentConfigDoc = QJsonDocument::fromJson(currentConfigData);
        if (currentConfigDoc.isObject()) {
            rootObj = currentConfigDoc.object();
        }
    } else {
        qDebug() << "管理员模块(AdminModule): 现有配置文件无法读取或不存在，将创建新的根对象。";
    }

    QJsonArray appsArray;
    for (const AppInfo &app : apps) {
        QJsonObject appObj;
        appObj["name"] = app.name;
        appObj["path"] = app.path;
        if (!app.mainExecutableHint.isEmpty()) {
            appObj["mainExecutableHint"] = app.mainExecutableHint;
        }
        if (!app.windowFindingHints.isEmpty()) {
            appObj["windowFindingHints"] = app.windowFindingHints; // Save the QJsonObject directly
        }
        // Note: Icon is not saved as a path here; it's dynamically loaded.
        // If a persistent icon_path was desired, it would be saved here.
        appsArray.append(appObj);
    }
    rootObj["whitelist_apps"] = appsArray; 
    qDebug() << "管理员模块(AdminModule):" << apps.size() << "个应用已序列化到JSON。";

    QFile configFileWrite(configPath);
    if (!configFileWrite.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行写入:" << configPath;
        return false;
    }
    
    QJsonDocument newDoc(rootObj);
    configFileWrite.write(newDoc.toJson());
    configFileWrite.close();
    qDebug() << "管理员模块(AdminModule): 白名单已成功保存到配置文件。";
    return true;
}

bool AdminModule::verifyPassword(const QString& password) {
    if (m_adminPasswordHash.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 密码哈希为空，验证失败。请检查配置加载。";
        return false; 
    }
    QString inputPasswordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    bool match = (inputPasswordHash == m_adminPasswordHash);
    qDebug() << "管理员模块(AdminModule): 验证密码 -" << (match ? "成功" : "失败");
    return match;
}

bool AdminModule::saveAdminPasswordToConfig(const QString& newPassword) {
    if (newPassword.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 不能将管理员密码设置为空。";
        return false;
    }
    m_adminPasswordHash = QString(QCryptographicHash::hash(newPassword.toUtf8(), QCryptographicHash::Sha256).toHex());
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QCoreApplication::applicationDirPath();
    }
    QString configPath = configDir + "/config.json";
    
    QFile configFile(configPath);
    QJsonObject rootObj;

    if (configFile.exists() && configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(configFile.readAll());
        if (existingDoc.isObject()) {
            rootObj = existingDoc.object();
        }
        configFile.close();
    }

    rootObj["admin_password"] = m_adminPasswordHash;

    QJsonDocument newDoc(rootObj);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件写入新密码: " << configFile.errorString();
        return false;
    }
    configFile.write(newDoc.toJson(QJsonDocument::Indented));
    configFile.close();
    qDebug() << "管理员模块(AdminModule): 新的管理员密码哈希已保存到配置文件。";
    return true;
}

bool AdminModule::saveAdminLoginHotkeyToConfig(const QStringList& hotkeyVkStrings) {
    if (hotkeyVkStrings.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 不能保存空的管理员登录热键。";
        return false;
    }

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
     if (configDir.isEmpty()) {
        configDir = QCoreApplication::applicationDirPath();
    }
    QString configPath = configDir + "/config.json";

    QFile configFile(configPath);
    QJsonObject rootObj;

    if (configFile.exists() && configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(configFile.readAll());
        if (existingDoc.isObject()) {
            rootObj = existingDoc.object();
        }
        configFile.close();
    }
    
    QJsonObject shortcutsObj;
    if (rootObj.contains("shortcuts") && rootObj["shortcuts"].isObject()) {
        shortcutsObj = rootObj["shortcuts"].toObject();
    }
    QJsonObject adminLoginObj;
    if (shortcutsObj.contains("admin_login") && shortcutsObj["admin_login"].isObject()) {
        adminLoginObj = shortcutsObj["admin_login"].toObject(); 
    }
    adminLoginObj["key_sequence"] = QJsonArray::fromStringList(hotkeyVkStrings);
    shortcutsObj["admin_login"] = adminLoginObj;
    rootObj["shortcuts"] = shortcutsObj;

    QJsonDocument newDoc(rootObj);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件写入新热键: " << configFile.errorString();
        return false;
    }
    configFile.write(newDoc.toJson(QJsonDocument::Indented));
    configFile.close();
    qDebug() << "管理员模块(AdminModule): 新的管理员登录热键 (字符串) 已保存到配置文件:" << hotkeyVkStrings.join(" + ");
    
    if(m_systemInteractionModulePtr) {
        m_systemInteractionModulePtr->loadConfiguration(); // Предположим, что SIM имеет loadConfiguration
    }
    return true;
}

QList<AppInfo> AdminModule::getWhitelistedApps() const
{
    return m_whitelistedApps; // Ensure this returns the member variable correctly
}

void AdminModule::prepareAdminDashboardData()
{
    qDebug() << "管理员模块(AdminModule): prepareAdminDashboardData() 调用。";
    if (m_adminDashboardView) {
        m_adminDashboardView->setWhitelistedApps(m_whitelistedApps);
        if (m_systemInteractionModulePtr) {
            m_adminDashboardView->setCurrentAdminLoginHotkey(m_systemInteractionModulePtr->getCurrentAdminLoginHotkeyStrings());
        } else {
            qWarning() << "AdminModule: SystemInteractionModule is null in prepareAdminDashboardData.";
            m_adminDashboardView->setCurrentAdminLoginHotkey(QStringList() << "Error: SIM null");
        }
        qDebug() << "管理员模块(AdminModule): 管理员仪表盘视图数据已准备就绪。";
    } else {
         qWarning() << "管理员模块(AdminModule): 管理员仪表盘视图实例为空，无法准备数据！";
    }
}