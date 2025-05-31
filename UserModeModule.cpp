#include "UserModeModule.h"
// #include "UserView.h" // Already included via UserModeModule.h if UserView is forward declared and then UserModeModule.h includes UserView.h
// #include "SystemInteractionModule.h" // Already included via UserModeModule.h
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
// #include <QProcess> // Already included via UserModeModule.h
#include <QThread>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QPointer> // Keep for QPointer if used, though m_systemInteractionModulePtr is raw now

// ========================= 构造与析构 =========================

/**
 * @brief 构造函数，初始化成员变量，连接信号槽，加载配置
 */
UserModeModule::UserModeModule(JianqiaoCoreShell *coreShell, UserView* userView, SystemInteractionModule* systemInteraction, QObject *parent)
    : QObject(parent),
      m_coreShellPtr(coreShell),
      m_userViewPtr(userView),
      m_systemInteractionModulePtr(systemInteraction),
      m_processMonitoringTimer(new QTimer(this)),
      m_configLoaded(false)
{
    // 检查指针有效性
    if (!m_coreShellPtr) {
        qCritical() << "UserModeModule: JianqiaoCoreShell pointer (m_coreShellPtr) is null!";
    }
    if (!m_systemInteractionModulePtr) {
        qCritical() << "UserModeModule: SystemInteractionModule pointer (m_systemInteractionModulePtr) is null!";
    }
    if (!m_userViewPtr) {
        qCritical() << "UserModeModule: UserView pointer (m_userViewPtr) is null!";
    }

    // 加载白名单配置
    loadConfiguration();
    if (m_userViewPtr) {
        // 连接UserView的应用启动信号
        disconnect(m_userViewPtr, &UserView::applicationLaunchRequested, this, nullptr);
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        // 连接白名单更新信号
        disconnect(this, &UserModeModule::userAppListUpdated, m_userViewPtr, nullptr);
        connect(this, &UserModeModule::userAppListUpdated, m_userViewPtr, &UserView::setAppList);
        qInfo() << "UserModeModule initialized and connected to UserView (app launch request and app list update).";
    } else {
        qWarning() << "UserModeModule: UserView is null, cannot connect signals.";
    }
    if (m_systemInteractionModulePtr) {
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::applicationActivated, this, &UserModeModule::onApplicationActivated);
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::applicationActivationFailed, this, &UserModeModule::onApplicationActivationFailed);
    }
    // 可选：启动进程监控定时器
    // connect(m_processMonitoringTimer, &QTimer::timeout, this, &UserModeModule::monitorLaunchedProcesses);
    // m_processMonitoringTimer->start(5000);
}

/**
 * @brief 析构函数，清理所有已启动进程，发射关闭信号
 */
UserModeModule::~UserModeModule()
{
    qInfo() << "UserModeModule destroyed.";
    for (QProcess* process : m_launchedProcesses.values()) {
        if (process && process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(1000)) {
                process->kill();
            }
        }
    }
    m_launchedProcesses.clear();
    emit userModeDeactivated();
}

// ========================= 用户模式激活/关闭 =========================

/**
 * @brief 激活用户模式，刷新白名单并显示UserView
 */
void UserModeModule::activate()
{
    qInfo() << "User mode activated.";
    loadAndSetWhitelist();
    if(m_userViewPtr) {
        m_userViewPtr->show();
        m_userViewPtr->update();
        m_userViewPtr->adjustSize();
    }
    emit userModeActivated();
}

/**
 * @brief 关闭用户模式，隐藏UserView
 */
void UserModeModule::deactivate()
{
    qInfo() << "User mode deactivated.";
    if(m_userViewPtr) {
        m_userViewPtr->hide();
    }
    emit userModeDeactivated();
}

// ========================= UserView显示/隐藏 =========================

/**
 * @brief 显示用户视图并刷新应用列表
 */
void UserModeModule::showUserView()
{
    if (m_userViewPtr) {
        qDebug() << "[UserModeModule::showUserView] Preparing to show UserView. Whitelisted app count in UserModeModule:" << m_whitelistedApps.count();
        m_userViewPtr->setAppList(m_whitelistedApps);
        m_userViewPtr->show();
        qInfo() << "UserModeModule: UserView shown.";
    } else {
        qWarning() << "UserModeModule::showUserView: m_userViewPtr is null!";
    }
}

/**
 * @brief 隐藏用户视图
 */
void UserModeModule::hideUserView()
{
    if (m_userViewPtr) {
        m_userViewPtr->hide();
        qInfo() << "UserModeModule: UserView hidden.";
    } else {
        qWarning() << "UserModeModule::hideUserView: m_userViewPtr is null!";
    }
}

/**
 * @brief 判断用户视图是否可见
 * @return 是否可见
 */
bool UserModeModule::isUserViewVisible() const
{
    if (m_userViewPtr) {
        return m_userViewPtr->isVisible();
    }
    qWarning() << "UserModeModule::isUserViewVisible: m_userViewPtr is null!";
    return false;
}

/**
 * @brief 获取用户视图QWidget指针
 */
QWidget* UserModeModule::getUserViewWidget()
{
    return m_userViewPtr;
}

/**
 * @brief 获取UserView实例指针
 */
UserView* UserModeModule::getViewInstance()
{
    return m_userViewPtr;
}

/**
 * @brief 设置UserView实例指针，并重新连接信号
 * @param view UserView指针
 */
void UserModeModule::setUserViewInstance(UserView* view)
{
    m_userViewPtr = view;
    if (m_userViewPtr) {
        disconnect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        qInfo() << "UserModeModule: UserView instance set and connected.";
    } else {
        qWarning() << "UserModeModule: setUserViewInstance called with a null view.";
    }
}

// ========================= 配置加载与白名单管理 =========================

/**
 * @brief 加载配置文件，填充白名单应用列表
 */
void UserModeModule::loadConfiguration()
{
    m_whitelistedApps.clear();
    QString configPath = UserModeModule::getConfigFilePath();
    QFile configFile(configPath);
    if (!QFileInfo::exists(configPath)) {
        qWarning() << "Config file not found at" << configPath << "Creating a default one.";
        return;
    }
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open config file for reading:" << configFile.errorString();
        return;
    }
    QByteArray jsonData = configFile.readAll();
    configFile.close();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Failed to parse config JSON or it's not an object.";
        return;
    }
    QJsonObject rootObj = doc.object();
    if (rootObj.contains("whitelist_apps") && rootObj["whitelist_apps"].isArray()) {
        QJsonArray appsArray = rootObj["whitelist_apps"].toArray();
        for (const QJsonValue &value : appsArray) {
            QJsonObject appObj = value.toObject();
            AppInfo app;
            app.name = appObj["name"].toString();
            app.path = appObj["path"].toString();
            app.mainExecutableHint = appObj["mainExecutableHint"].toString();
            app.smartTopmost = appObj["smartTopmost"].toBool();
            app.forceTopmost = appObj["forceTopmost"].toBool();
            qDebug() << "UserModeModule::loadConfiguration - Loaded app:" << app.name << "Path:" << app.path << "Hint:" << app.mainExecutableHint << "SmartTopmost:" << app.smartTopmost << "ForceTopmost:" << app.forceTopmost;
            if (m_systemInteractionModulePtr) {
                app.icon = m_systemInteractionModulePtr->getIconForExecutable(app.path);
            } else {
                qWarning() << "SystemInteractionModule is null, cannot fetch icon for" << app.name;
                if (appObj.contains("icon_path")) {
                    app.icon = QIcon(appObj["icon_path"].toString());
                    if (app.icon.isNull()) {
                        qWarning() << "Failed to load icon from icon_path:" << appObj["icon_path"].toString();
                    }
                }
            }
            if (app.icon.isNull()) {
                qWarning() << "Icon for" << app.name << "is null. Path:" << app.path;
            }
            m_whitelistedApps.append(app);
        }
    }
    // 加载完成后刷新UserView和发射信号
    if (m_userViewPtr) {
        m_userViewPtr->setAppList(m_whitelistedApps);
    }
    emit userAppListUpdated(m_whitelistedApps);
    qInfo() << "Configuration loaded," << m_whitelistedApps.count() << "apps in whitelist.";
}

/**
 * @brief 加载配置并刷新UserView
 */
void UserModeModule::loadAndSetWhitelist()
{
    loadConfiguration();
    if (m_userViewPtr) {
        m_userViewPtr->setAppList(m_whitelistedApps);
    }
    emit userAppListUpdated(m_whitelistedApps);
}

/**
 * @brief 终止所有已启动的进程
 */
void UserModeModule::terminateActiveProcesses() {
    qInfo() << "UserModeModule: Terminating all active/launched processes.";
    for (QProcess* process : m_launchedProcesses.values()) {
        if (process && process->state() != QProcess::NotRunning) {
            QString appPath = m_launchedProcesses.key(process);
            qInfo() << "Terminating process for app:" << (appPath.isEmpty() ? "Unknown" : appPath);
            process->terminate();
            if (!process->waitForFinished(1000)) {
                process->kill();
                qInfo() << "Process for" << (appPath.isEmpty() ? "Unknown" : appPath) << "killed.";
            } else {
                qInfo() << "Process for" << (appPath.isEmpty() ? "Unknown" : appPath) << "terminated gracefully.";
            }
        }
    }
    m_launchedProcesses.clear();
    qInfo() << "Cleared tracked processes.";
}

// ========================= 应用启动与进程管理 =========================

/**
 * @brief 启动指定应用进程，并监控其状态
 * @param appPath 应用路径
 * @param appName 应用名称
 */
void UserModeModule::launchApplication(const QString& appPath, const QString& appName) {
    if (appPath.isEmpty()) {
        qWarning() << "UserModeModule: Application path is empty, cannot launch.";
        return;
    }
    qInfo() << "UserModeModule: Attempting to launch" << appName << "at" << appPath;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, true);
    }
    QProcess *process = new QProcess(this);
    m_launchedProcesses.insert(appPath, process);
    // 进程结束信号处理
    connect(process, &QProcess::finished, this, [this, process, appPath, appName](int exitCode, QProcess::ExitStatus exitStatus) {
        qInfo() << "UserModeModule: Application" << appName << "(" << appPath << ") finished. Exit code:" << exitCode << "Exit status:" << static_cast<int>(exitStatus);
        m_launchedProcesses.remove(appPath);
        process->deleteLater();
        if (m_userViewPtr) {
            m_userViewPtr->setAppLoadingState(appPath, false);
        }
        if (m_systemInteractionModulePtr) {
            m_systemInteractionModulePtr->stopMonitoringProcess(appPath);
        }
    });
    // 进程错误信号处理
    connect(process, &QProcess::errorOccurred, this, [this, process, appPath, appName](QProcess::ProcessError error) {
        qWarning() << "UserModeModule: Error launching" << appName << "(" << appPath << "). Error:" << error;
        m_launchedProcesses.remove(appPath);
        process->deleteLater();
        if (m_userViewPtr) {
            m_userViewPtr->setAppLoadingState(appPath, false);
        }
        if (m_systemInteractionModulePtr) {
            m_systemInteractionModulePtr->stopMonitoringProcess(appPath);
        }
    });
    process->setProgram(appPath);
    process->start();
    if (!process->waitForStarted(5000)) {
        qWarning() << "UserModeModule: Process" << appPath << "failed to start or timed out starting.";
        QProcess::ProcessError error = process->error();
        qWarning() << "UserModeModule: QProcess error:" << error << process->errorString();
        m_launchedProcesses.remove(appPath);
        process->deleteLater();
        if (m_userViewPtr) {
            m_userViewPtr->setAppLoadingState(appPath, false);
        }
        return;
    }
    qInfo() << "UserModeModule: Process" << appPath << "started with PID:" << process->processId();
    // 启动后调用系统交互模块监控窗口
    if (m_systemInteractionModulePtr) {
        AppInfo appInfoToFind;
        for(const auto& ai : m_whitelistedApps) {
            if (ai.path == appPath) {
                appInfoToFind = ai;
                break;
            }
        }
        if (appInfoToFind.path.isEmpty()) {
            qWarning() << "UserModeModule: Could not find AppInfo for" << appPath << "to pass to monitorAndActivateApplication.";
            if (m_userViewPtr) {
                m_userViewPtr->setAppLoadingState(appPath, false);
            }
            return;
        }
        // 同步置顶策略到系统交互模块
        if (m_systemInteractionModulePtr) {
            m_systemInteractionModulePtr->setSmartTopmostEnabled(appInfoToFind.smartTopmost);
            m_systemInteractionModulePtr->setForceTopmostEnabled(appInfoToFind.forceTopmost);
        }
        DWORD pid = static_cast<DWORD>(process->processId());
        qDebug() << "UserModeModule: Calling monitorAndActivateApplication for PID:" << pid << "AppInfo Name:" << appInfoToFind.name;
        m_systemInteractionModulePtr->monitorAndActivateApplication(appPath, 
                                                                  static_cast<quint32>(pid), 
                                                                  appInfoToFind.mainExecutableHint, 
                                                                  appInfoToFind.windowFindingHints);
    } else {
        qWarning() << "UserModeModule: m_systemInteractionModulePtr is null, cannot monitor/activate window for" << appPath;
        if (m_userViewPtr) {
            m_userViewPtr->setAppLoadingState(appPath, false);
        }
    }
}

/**
 * @brief 处理UserView发来的应用启动请求信号
 * @param appPath 应用路径
 * @param appName 应用名称
 */
void UserModeModule::onApplicationLaunchRequested(const QString& appPath, const QString& appName) {
    qDebug() << "UserModeModule::onApplicationLaunchRequested - appPath:" << appPath << ", appName:" << appName;
    if (m_launchedProcesses.contains(appPath)) {
        qInfo() << "UserModeModule: Application" << appName << "is already running or being launched.";
        if(m_systemInteractionModulePtr && m_launchedProcesses.value(appPath)->state() == QProcess::Running) {
            qDebug() << "UserModeModule: Attempting to re-activate already running process:" << appName;
            AppInfo appInfoToFind;
            for(const auto& ai : m_whitelistedApps) {
                if (ai.path == appPath) {
                    appInfoToFind = ai;
                    break;
                }
            }
            if(!appInfoToFind.path.isEmpty()){
                if (m_userViewPtr) {
                    m_userViewPtr->setAppLoadingState(appPath, true);
                }
                m_systemInteractionModulePtr->monitorAndActivateApplication(appPath, 
                                                                          0, // 重新激活已知进程
                                                                          appInfoToFind.mainExecutableHint, 
                                                                          appInfoToFind.windowFindingHints, 
                                                                          true); // forceActivateOnly = true
            } else {
                qWarning() << "UserModeModule: Could not find AppInfo for re-activation of" << appPath;
            }
        }
        return;
    }
    launchApplication(appPath, appName);
}

/**
 * @brief 应用窗口激活成功槽，取消加载中并高亮状态栏
 * @param appPath 应用路径
 */
void UserModeModule::onApplicationActivated(const QString& appPath) {
    qInfo() << "UserModeModule::onApplicationActivated - Application" << appPath << "has been activated.";
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activated):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false);
        m_userViewPtr->setActiveAppInStatusBar(appPath);
    }
}

/**
 * @brief 应用窗口激活失败槽，取消加载中
 * @param appPath 应用路径
 */
void UserModeModule::onApplicationActivationFailed(const QString& appPath) {
    qWarning() << "UserModeModule::onApplicationActivationFailed - Activation failed for" << appPath;
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activation failed):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false);
    }
}

// ========================= 进程相关槽函数 =========================

/**
 * @brief 进程启动完成槽
 * @param appPath 应用路径
 */
void UserModeModule::onProcessStarted(const QString& appPath) {
    qDebug() << "UserModeModule::onProcessStarted (Restored) - Process started for:" << appPath;
}

/**
 * @brief 进程结束槽
 * @param appPath 应用路径
 * @param exitCode 退出码
 * @param exitStatus 退出状态
 */
void UserModeModule::onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "UserModeModule::onProcessFinished (Legacy Slot) - Process for" << appPath
              << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false);
    }
}

/**
 * @brief 进程错误槽
 * @param appPath 应用路径
 * @param error 错误类型
 */
void UserModeModule::onProcessError(const QString& appPath, QProcess::ProcessError error) {
    qWarning() << "UserModeModule::onProcessError (Legacy Slot) - Error for process" << appPath << "Error:" << error;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false);
    }
}

/**
 * @brief 进程状态变化槽，调试用
 * @param newState 新状态
 */
void UserModeModule::onProcessStateChanged(QProcess::ProcessState newState) {
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    QString appPath;
    for(auto it = m_launchedProcesses.constBegin(); it != m_launchedProcesses.constEnd(); ++it) {
        if (it.value() == process) {
            appPath = it.key();
            break;
        }
    }
    qDebug() << "UserModeModule::onProcessStateChanged - Process for" << (appPath.isEmpty() ? "unknown app" : appPath)
             << "changed state to:" << newState;
}

// ========================= 工具与辅助方法 =========================

/**
 * @brief 判断某可执行文件路径是否在白名单
 * @param executablePath 可执行文件路径
 * @return 是否在白名单
 */
bool UserModeModule::isAppWhitelisted(const QString& executablePath) const
{
    for (const auto& app : m_whitelistedApps) {
        if (QFileInfo(app.path).fileName().compare(QFileInfo(executablePath).fileName(), Qt::CaseInsensitive) == 0 ||
            app.path.compare(executablePath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 根据应用名查找路径
 * @param appName 应用名
 * @return 路径
 */
QString UserModeModule::getAppPathForName(const QString& appName) const
{
    for (const auto& app : m_whitelistedApps) {
        if (app.name.compare(appName, Qt::CaseInsensitive) == 0) {
            return app.path;
        }
    }
    return QString();
}

/**
 * @brief 启动进程监控定时器
 */
void UserModeModule::startProcessMonitoringTimer() {
    if (!m_processMonitoringTimer) {
        m_processMonitoringTimer = new QTimer(this);
    }
    if (!m_processMonitoringTimer->isActive()) {
        connect(m_processMonitoringTimer, &QTimer::timeout, this, &UserModeModule::monitorLaunchedProcesses);
        m_processMonitoringTimer->start(15000);
        qInfo() << "Process monitoring timer started.";
    }
}

/**
 * @brief 定时监控所有已启动进程，自动清理已结束进程
 */
void UserModeModule::monitorLaunchedProcesses() {
    auto it = m_launchedProcesses.begin();
    while (it != m_launchedProcesses.end()) {
        QProcess* process = it.value();
        QString appPath = it.key();
        if (!process || process->state() == QProcess::NotRunning) {
            qInfo() << "Monitored process" << appPath << "is no longer running or invalid. Removing.";
            it = m_launchedProcesses.erase(it);
            if(process) process->deleteLater();
        } else {
            ++it;
        }
    }
    if (m_launchedProcesses.isEmpty() && m_processMonitoringTimer && m_processMonitoringTimer->isActive()){
        // m_processMonitoringTimer->stop();
    }
}

/**
 * @brief 根据进程指针查找应用路径
 * @param process 进程指针
 * @return 应用路径
 */
QString UserModeModule::findAppPathForProcess(QProcess* process) {
    for(auto it = m_launchedProcesses.constBegin(); it != m_launchedProcesses.constEnd(); ++it) {
        if (it.value() == process) {
            return it.key();
        }
    }
    return QString();
}

/**
 * @brief 更新白名单应用列表并刷新UserView
 * @param apps 新的应用列表
 */
void UserModeModule::updateUserAppList(const QList<AppInfo>& apps) {
    qDebug() << "UserModeModule::updateUserAppList - Updating apps from provided list. Count:" << apps.count();
    m_whitelistedApps.clear();
    for (AppInfo app : apps) {
        // 如果mainExecutableHint为空，自动补全为可执行文件名
        if (app.mainExecutableHint.isEmpty() && !app.path.isEmpty()) {
            QFileInfo fi(app.path);
            app.mainExecutableHint = fi.fileName();
        }
        m_whitelistedApps.append(app);
    }
    if (m_userViewPtr) {
        m_userViewPtr->setAppList(m_whitelistedApps);
    }
    emit userAppListUpdated(m_whitelistedApps);
    qDebug() << "UserModeModule::updateUserAppList - Signal emitted to update UserView with new app list.";

    // 修复：写入 config.json 时先读取原有内容，合并 whitelist_apps 字段后再写回，避免覆盖其它字段。
    QString configPath = UserModeModule::getConfigFilePath();
    QJsonObject rootObj;
    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument oldDoc = QJsonDocument::fromJson(configFile.readAll());
        if (oldDoc.isObject()) {
            rootObj = oldDoc.object();
        }
        configFile.close();
    }
    QJsonArray appsArray;
    for (const AppInfo& app : m_whitelistedApps) {
        QJsonObject appObj;
        appObj["name"] = app.name;
        appObj["path"] = app.path;
        appObj["mainExecutableHint"] = app.mainExecutableHint;
        appObj["smartTopmost"] = app.smartTopmost;
        appObj["forceTopmost"] = app.forceTopmost;
        appsArray.append(appObj);
    }
    rootObj["whitelist_apps"] = appsArray;
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QJsonDocument newDoc(rootObj);
        configFile.write(newDoc.toJson(QJsonDocument::Indented));
        configFile.close();
    }
}

/**
 * @brief 静态函数：统一获取配置文件路径，始终使用当前登录用户的USERPROFILE目录，避免管理员/普通用户路径不一致
 * @return 配置文件绝对路径
 */
QString UserModeModule::getConfigFilePath() {
    // 通过环境变量USERPROFILE获取当前登录用户主目录，确保所有身份下配置一致
    QString userProfile = qEnvironmentVariable("USERPROFILE");
    QString configDir = userProfile + "/AppData/Roaming/雪鸮团队/剑鞘系统";
    QDir().mkpath(configDir); // 确保目录存在
    return configDir + "/config.json";
} 