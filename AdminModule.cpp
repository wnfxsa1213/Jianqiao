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
      m_systemInteractionModulePtr(systemInteraction)
{
    qDebug() << "管理员模块(AdminModule): 构造函数开始。";

    loadConfig(); 

    connect(m_loginView, &AdminLoginView::loginAttempt, this, &AdminModule::onLoginAttempt);
    connect(m_loginView, &AdminLoginView::userRequestsExit, this, &AdminModule::onLoginViewRequestsExit);
    connect(m_loginView, &AdminLoginView::openWhitelistManagerRequested, this, &AdminModule::showAdminDashboardView);

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

void AdminModule::showAdminDashboardView()
{
    qDebug() << "管理员模块(AdminModule): 请求显示管理员仪表盘视图。";
    if (m_loginView && m_loginView->isVisible()) {
        m_loginView->hide();
    }

    if (!m_adminDashboardView) { 
        qWarning() << "管理员模块(AdminModule): 管理员仪表盘视图为空，尝试重新创建。";
        // m_adminDashboardView = new AdminDashboardView(nullptr);
        // connect(m_adminDashboardView, &AdminDashboardView::whitelistChanged, this, &AdminModule::onWhitelistUpdated);
        // connect(m_adminDashboardView, &AdminDashboardView::userRequestsExitAdminMode, this, &AdminModule::onUserRequestsExitAdminMode);
        // connect(m_adminDashboardView, &AdminDashboardView::changePasswordRequested, this, &AdminModule::onChangePasswordRequested);
        // connect(m_adminDashboardView, &AdminDashboardView::adminLoginHotkeyChanged, this, &AdminModule::onAdminLoginHotkeyChanged);
    }

    if (m_adminDashboardView) {
        m_adminDashboardView->setWhitelistedApps(m_whitelistedApps);
        m_adminDashboardView->setCurrentAdminLoginHotkey(m_systemInteractionModulePtr->getCurrentAdminLoginHotkeyStrings());
        qDebug() << "管理员模块(AdminModule): 管理员仪表盘视图数据已准备就绪。显示由CoreShell的StackedWidget处理。";
        // if (!m_adminDashboardView->isVisible()) { // CoreShell handles visibility
        //     m_adminDashboardView->showNormal(); 
        // }
        // m_adminDashboardView->activateWindow(); // CoreShell handles activation/focus
        // m_adminDashboardView->raise(); // CoreShell handles Z-order within its stack
        // emit adminViewVisible(true); // REMOVED: Dashboard is part of stacked widget, shell's Z-order shouldn't change for this.
    } else {
         qWarning() << "管理员模块(AdminModule): 管理员仪表盘视图实例为空，无法准备数据！";
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
    qDebug() << "管理员模块(AdminModule): AdminDashboardView 请求退出管理模式。";
    // if (m_adminDashboardView) { // Visibility managed by StackedWidget
    //     m_adminDashboardView->hide();
    // }
    emit adminViewVisible(false); 
    emit exitAdminModeRequested(); 
}

void AdminModule::onLoginViewRequestsExit()
{
    qDebug() << "管理员模块(AdminModule): 登录视图请求退出管理员模式（例如通过取消按钮）。";
    if (m_loginView) {
        m_loginView->hide();
    }
    if (!isAnyViewVisible()) {
         emit adminViewVisible(false); 
         emit exitAdminModeRequested();
    }
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
        if(m_loginView) {
            m_loginView->notifyLoginResult(true); 
            // Potentially hide login view *after* CoreShell has switched to dashboard if there's a flicker
            // For now, let login view hide itself or CoreShell manage it upon mode change.
            // m_loginView->hide(); // Consider if this is needed here or if openWhitelistManagerRequested signal should do it.
        }
        // This signal will cause CoreShell to switch to the AdminDashboardView
        emit loginSuccessfulAndAdminActive(); 
        // This call now mainly ensures the dashboard has the latest data before CoreShell shows it.
        showAdminDashboardView(); 
    } else {
        qDebug() << "管理员模块(AdminModule): 密码验证失败。";
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

void AdminModule::onWhitelistUpdated(const QList<AppInfo>& updatedWhitelist)
{
    qDebug() << "AdminModule: Whitelist updated in view with" << updatedWhitelist.size() << "apps. Processing and saving to config.";
    m_whitelistedApps = updatedWhitelist;

    // Attempt to fetch/refresh icons using SystemInteractionModule before saving and updating view further
    if (m_systemInteractionModulePtr) {
        qDebug() << "AdminModule: Refreshing icons for the updated whitelist...";
        for (AppInfo& app : m_whitelistedApps) { // Use reference to modify original AppInfo objects
            // Only fetch if icon is null or perhaps from a known default/placeholder source if you add such logic
            // For simplicity, let's always try to get the best icon via SIM here.
            QIcon newIcon = m_systemInteractionModulePtr->getIconForExecutable(app.path);
            if (!newIcon.isNull()) {
                app.icon = newIcon;
            } else {
                qWarning() << "AdminModule: SIM failed to get icon for" << app.path << ". Current icon in AppInfo might be from QIcon(path) or null.";
            }
        }
    }

    // Now that m_whitelistedApps might have updated icons, update the dashboard view again
    // This ensures the dashboard shows icons fetched by SystemInteractionModule if they were initially missing or basic.
    if (m_adminDashboardView) {
        qDebug() << "AdminModule: Resending app list to dashboard view with potentially updated icons.";
        m_adminDashboardView->setWhitelistedApps(m_whitelistedApps);
    }

    if (!saveWhitelistToConfig(m_whitelistedApps)) {
        qWarning() << "AdminModule: Failed to save updated whitelist to config.";
        QMessageBox::critical(m_adminDashboardView, "保存失败", "无法将白名单更改保存到配置文件。");
        if(m_adminDashboardView) {
             // Re-load from config and set to view to revert any optimistic UI changes
             loadConfig(); // This already calls m_adminDashboardView->setWhitelistedApps internally if view exists
        }
    } else {
         qDebug() << "AdminModule: Whitelist successfully saved to config.json.";
         emit configurationChanged(); 
    }
}

void AdminModule::loadConfig() {
    qDebug() << "管理员模块(AdminModule): 开始加载配置 (密码, 热键, 白名单)...";
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 无法获取应用配置目录路径!";
        configDir = QCoreApplication::applicationDirPath(); 
        qWarning() << "管理员模块(AdminModule): 回退到应用可执行文件目录:" << configDir;
    }
    
    QDir dir(configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "管理员模块(AdminModule): 无法创建配置目录:" << configDir;
            initializeDefaultAdminHotkey(); 
            return;
        }
        qDebug() << "管理员模块(AdminModule): 配置目录已创建:" << configDir;
    }

    QString configPath = configDir + "/config.json";
    qDebug() << "管理员模块(AdminModule): 完整的配置文件路径:" << configPath;

    QFile configFile(configPath);
    if (!configFile.exists()) {
        qWarning() << "管理员模块(AdminModule): 配置文件" << configPath << "不存在。将尝试创建默认配置。";
        m_adminPasswordHash = QString(QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex());
        initializeDefaultAdminHotkey(); // This sets m_currentAdminLoginHotkeyVkCodes
        m_whitelistedApps.clear();
        saveConfig(); 
        if (!configFile.exists()) {
             qCritical() << "管理员模块(AdminModule): 无法创建默认配置文件！";
             return;
        }
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行读取: " << configFile.errorString();
        initializeDefaultAdminHotkey();
        return;
    }

    QByteArray jsonData = configFile.readAll();
    configFile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "管理员模块(AdminModule): 解析配置文件JSON失败或不是一个对象。";
        initializeDefaultAdminHotkey(); // Fallback to defaults
        m_adminPasswordHash = QString(QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex());
        m_whitelistedApps.clear();
        return;
    }

    QJsonObject rootObj = jsonDoc.object();

    if (rootObj.contains("admin_password_hash") && rootObj["admin_password_hash"].isString()) {
        m_adminPasswordHash = rootObj["admin_password_hash"].toString();
        qDebug() << "管理员模块(AdminModule): 管理员密码哈希已从配置加载。";
    } else {
        qWarning() << "管理员模块(AdminModule): 配置文件中未找到或无效的 'admin_password_hash'。将使用默认密码 'admin'。";
        m_adminPasswordHash = QString(QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex());
    }

    if (rootObj.contains("shortcuts") && rootObj["shortcuts"].isObject()) {
        QJsonObject shortcutsObj = rootObj["shortcuts"].toObject();
        if (shortcutsObj.contains("admin_login") && shortcutsObj["admin_login"].isObject()) {
            QJsonObject adminLoginObj = shortcutsObj["admin_login"].toObject();
            if (adminLoginObj.contains("key_sequence") && adminLoginObj["key_sequence"].isArray()) {
                QJsonArray hotkeyArray = adminLoginObj["key_sequence"].toArray();
                QStringList vkStrings;
                for (const QJsonValue& val : hotkeyArray) {
                    if (val.isString()) {
                        vkStrings.append(val.toString());
                    }
                }
                if (!vkStrings.isEmpty()) {
                    qDebug() << "管理员模块(AdminModule): 管理员登录热键 (VK字符串) 已从配置加载:" << vkStrings.join(" + ");
                    // m_currentAdminLoginHotkeyVkCodes 应该由 SystemInteractionModule 自己加载和管理
                    // AdminModule 主要关心这个字符串序列是否能被SIM正确解析
                    // 此处可以考虑存储这个 vkStrings 以便传递给 HotkeyEditDialog 进行显示
                    // m_loadedAdminHotkeyStrings = vkStrings; // 示例：如果需要存储
                } else {
                    qWarning() << "管理员模块(AdminModule): 配置文件中 'shortcuts.admin_login.key_sequence' 为空或无效。";
                    initializeDefaultAdminHotkey();
                }
            } else {
                qWarning() << "管理员模块(AdminModule): 配置文件中未找到或无效的 'shortcuts.admin_login.key_sequence'。";
                initializeDefaultAdminHotkey();
            }
        } else {
            qWarning() << "管理员模块(AdminModule): 配置文件中未找到或无效的 'shortcuts.admin_login' 对象。";
            initializeDefaultAdminHotkey();
        }
    } else {
        qWarning() << "管理员模块(AdminModule): 配置文件中未找到 'shortcuts' 对象。将使用旧的 'admin_login_hotkey_vk_strings' 逻辑或默认热键。";
        // 保留旧的直接读取 admin_login_hotkey_vk_strings 的逻辑作为回退，或者完全移除
        if (rootObj.contains("admin_login_hotkey_vk_strings") && rootObj["admin_login_hotkey_vk_strings"].isArray()) {
            QJsonArray hotkeyArray = rootObj["admin_login_hotkey_vk_strings"].toArray();
            QStringList vkStrings;
            for (const QJsonValue& val : hotkeyArray) {
                if (val.isString()) {
                    vkStrings.append(val.toString());
                }
            }
            if (!vkStrings.isEmpty()) {
                qDebug() << "管理员模块(AdminModule): 管理员登录热键 (VK字符串，旧格式) 已从配置加载:" << vkStrings.join(" + ");
            } else {
                qWarning() << "管理员模块(AdminModule): 配置文件中 'admin_login_hotkey_vk_strings' (旧格式) 为空或无效。";
                initializeDefaultAdminHotkey(); // 确保如果新旧格式都失败，则使用默认
            }
        } else {
             // 如果连旧的字段也没有，并且新的shortcuts方式也失败了
            qWarning() << "管理员模块(AdminModule): 配置文件中未找到任何管理员登录热键配置。";
            initializeDefaultAdminHotkey();
        }
    }
    
    m_whitelistedApps.clear();
    if (rootObj.contains("whitelist_apps") && rootObj["whitelist_apps"].isArray()) {
        QJsonArray appsArray = rootObj["whitelist_apps"].toArray();
        for (const QJsonValue &value : appsArray) {
            QJsonObject appObj = value.toObject();
            AppInfo app;
            app.name = appObj["name"].toString();
            app.path = appObj["path"].toString();
            
            if (m_systemInteractionModulePtr) {
                app.icon = m_systemInteractionModulePtr->getIconForExecutable(app.path);
            } else if (appObj.contains("icon_path")) { 
                app.icon = QIcon(appObj["icon_path"].toString());
                 if (app.icon.isNull()) {
                    qWarning() << "AdminModule: Failed to load icon from icon_path for" << app.name << ":" << appObj["icon_path"].toString();
                }
            }
            if (app.icon.isNull()) {
                 qWarning() << "AdminModule: Icon for" << app.name << "is null after attempting load. Path:" << app.path;
            }
            m_whitelistedApps.append(app);
        }
        qDebug() << "管理员模块(AdminModule): 从配置文件加载了" << m_whitelistedApps.size() << "个白名单应用。";
    } else {
        qDebug() << "管理员模块(AdminModule): 配置文件中未找到 'whitelist_apps' 或格式不正确。白名单为空。";
    }

    if (m_adminDashboardView) { // Update view if it exists
        m_adminDashboardView->setWhitelistedApps(m_whitelistedApps);
    }
     qDebug() << "管理员模块(AdminModule): 配置加载完成。";
}

void AdminModule::saveConfig() {
    qDebug() << "管理员模块(AdminModule): 准备保存配置到JSON...";
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 无法获取应用配置目录路径! 将尝试保存到应用可执行文件目录。";
        configDir = QCoreApplication::applicationDirPath();
    }
    QString configPath = configDir + "/config.json";
    qDebug() << "管理员模块(AdminModule): 配置文件将保存到:" << configPath;

    QFile configFile(configPath);
    QJsonObject rootObj;

    if (configFile.exists() && configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(configFile.readAll());
        if (existingDoc.isObject()) {
            rootObj = existingDoc.object();
        }
        configFile.close();
    } else {
        qInfo() << "管理员模块(AdminModule): 现有配置文件未找到或无法读取，将创建新的。";
    }

    if (m_adminPasswordHash.isEmpty()){ 
         m_adminPasswordHash = QString(QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex());
         qWarning() << "管理员模块(AdminModule): 保存配置时发现密码哈希为空，已设置为默认 'admin' 的哈希。";
    }
    rootObj["admin_password_hash"] = m_adminPasswordHash;

    // 修改热键保存逻辑
    QJsonObject shortcutsObj;
    if (rootObj.contains("shortcuts") && rootObj["shortcuts"].isObject()) {
        shortcutsObj = rootObj["shortcuts"].toObject();
    }
    QJsonObject adminLoginObj;
    if (shortcutsObj.contains("admin_login") && shortcutsObj["admin_login"].isObject()) {
        adminLoginObj = shortcutsObj["admin_login"].toObject(); 
    }
    // adminLoginObj["key_sequence"] 会在 saveAdminLoginHotkeyToConfig 中更新
    // 这里确保基本结构存在
    shortcutsObj["admin_login"] = adminLoginObj;
    rootObj["shortcuts"] = shortcutsObj;

    // 移除旧的 admin_login_hotkey_vk_strings 保存逻辑 (如果存在的话)
    // rootObj.remove("admin_login_hotkey_vk_strings"); 

    if (!rootObj.contains("whitelist_apps")) {
        QJsonArray appsArray;
        for (const AppInfo& app : m_whitelistedApps) { // Use current m_whitelistedApps
            QJsonObject appObj;
            appObj["name"] = app.name;
            appObj["path"] = app.path;
            appsArray.append(appObj);
        }
        rootObj["whitelist_apps"] = appsArray;
    }

    QJsonDocument newDoc(rootObj);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行写入: " << configFile.errorString();
        return;
    }
    configFile.write(newDoc.toJson(QJsonDocument::Indented));
    configFile.close();
    qDebug() << "管理员模块(AdminModule): 配置已保存到" << configPath;
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
    qDebug() << "管理员模块(AdminModule): 准备将白名单保存到配置文件...";
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        qWarning() << "管理员模块(AdminModule): 无法获取应用配置目录路径! 将尝试保存到应用可执行文件目录。";
        configDir = QCoreApplication::applicationDirPath();
    }
    QString configPath = configDir + "/config.json";
    qDebug() << "管理员模块(AdminModule): 配置文件路径:" << configPath;

    QFile configFile(configPath);
    QJsonObject rootObj;

    if (configFile.exists() && configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument existingDoc = QJsonDocument::fromJson(configFile.readAll());
        if (existingDoc.isObject()) {
            rootObj = existingDoc.object();
        }
        configFile.close();
    }

    QJsonArray appsArray;
    for (const AppInfo& app : apps) {
        QJsonObject appObj;
        appObj["name"] = app.name;
        appObj["path"] = app.path;
        appsArray.append(appObj);
    }
    rootObj["whitelist_apps"] = appsArray; 

    QJsonDocument newDoc(rootObj);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "管理员模块(AdminModule): 无法打开配置文件进行写入: " << configFile.errorString();
        return false;
    }

    qint64 bytesWritten = configFile.write(newDoc.toJson(QJsonDocument::Indented));
    configFile.close();

    if (bytesWritten == -1) {
        qWarning() << "管理员模块(AdminModule): 写入配置文件失败!";
        return false;
    }
    
    qDebug() << "管理员模块(AdminModule): 白名单已成功写入配置文件 (" << bytesWritten << "字节)。";
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

    rootObj["admin_password_hash"] = m_adminPasswordHash;

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