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

// Constructor
UserModeModule::UserModeModule(JianqiaoCoreShell *coreShell, UserView* userView, SystemInteractionModule* systemInteraction, QObject *parent)
    : QObject(parent),
      m_coreShellPtr(coreShell), // Initialize m_coreShellPtr
      m_userViewPtr(userView),    // Changed m_userView to m_userViewPtr and initialized
      m_systemInteractionModulePtr(systemInteraction), // Initialize m_systemInteractionModulePtr
      m_processMonitoringTimer(new QTimer(this)),
      m_configLoaded(false)
{
    if (!m_coreShellPtr) {
        qCritical() << "UserModeModule: JianqiaoCoreShell pointer (m_coreShellPtr) is null!";
    }
    if (!m_systemInteractionModulePtr) {
        qCritical() << "UserModeModule: SystemInteractionModule pointer (m_systemInteractionModulePtr) is null!";
    }
    if (!m_userViewPtr) { // Changed m_userView to m_userViewPtr
        qCritical() << "UserModeModule: UserView pointer (m_userViewPtr) is null!";
    }

    loadConfiguration();
    if (m_userViewPtr) { 
        // Connect to UserView signals (ensure onApplicationLaunchRequested has two QStrings now)
        disconnect(m_userViewPtr, &UserView::applicationLaunchRequested, this, nullptr); // Disconnect any previous to avoid duplicates
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        qInfo() << "UserModeModule initialized and connected to UserView.";
    } else {
        qWarning() << "UserModeModule: UserView is null, cannot connect signals.";
    }

    if (m_systemInteractionModulePtr) {
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::applicationActivated, this, &UserModeModule::onApplicationActivated);
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::applicationActivationFailed, this, &UserModeModule::onApplicationActivationFailed);
    }

    // Connect signals from UserView
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested); // Changed m_userView to m_userViewPtr
    } else {
        qWarning() << "UserView is null in UserModeModule constructor.";
    }

    // connect(m_processMonitoringTimer, &QTimer::timeout, this, &UserModeModule::monitorLaunchedProcesses);
    // m_processMonitoringTimer->start(5000); // Check every 5 seconds
}

UserModeModule::~UserModeModule()
{
    qInfo() << "UserModeModule destroyed.";
    for (QProcess* process : m_launchedProcesses.values()) { // Iterate over values of QHash
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

void UserModeModule::activate()
{
    qInfo() << "User mode activated.";
    loadAndSetWhitelist(); // Refresh app list when activated
    if(m_userViewPtr) { // Changed m_userView to m_userViewPtr
        m_userViewPtr->show(); // Changed m_userView to m_userViewPtr
        m_userViewPtr->update();      // Force a repaint // Changed m_userView to m_userViewPtr
        m_userViewPtr->adjustSize();  // Adjust size if necessary due to new content // Changed m_userView to m_userViewPtr
    }
    emit userModeActivated();
}

void UserModeModule::deactivate()
{
    qInfo() << "User mode deactivated.";
    if(m_userViewPtr) { // Changed m_userView to m_userViewPtr
        m_userViewPtr->hide(); // Changed m_userView to m_userViewPtr
    }
    // Optionally, could terminate whitelisted apps here, or leave them running
    emit userModeDeactivated();
}

void UserModeModule::showUserView()
{
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        qDebug() << "[UserModeModule::showUserView] Preparing to show UserView. Whitelisted app count in UserModeModule:" << m_whitelistedApps.count();
        m_userViewPtr->setAppList(m_whitelistedApps); // Changed m_userView to m_userViewPtr

        m_userViewPtr->show(); // Changed m_userView to m_userViewPtr
        qInfo() << "UserModeModule: UserView shown.";
    } else {
        qWarning() << "UserModeModule::showUserView: m_userViewPtr is null!"; // Changed m_userView to m_userViewPtr
    }
}

void UserModeModule::hideUserView()
{
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        m_userViewPtr->hide(); // Changed m_userView to m_userViewPtr
        qInfo() << "UserModeModule: UserView hidden.";
    } else {
        qWarning() << "UserModeModule::hideUserView: m_userViewPtr is null!"; // Changed m_userView to m_userViewPtr
    }
}

bool UserModeModule::isUserViewVisible() const
{
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        return m_userViewPtr->isVisible(); // Changed m_userView to m_userViewPtr
    }
    qWarning() << "UserModeModule::isUserViewVisible: m_userViewPtr is null!"; // Changed m_userView to m_userViewPtr
    return false;
}

QWidget* UserModeModule::getUserViewWidget()
{
    return m_userViewPtr; // Changed m_userView to m_userViewPtr
}

UserView* UserModeModule::getViewInstance()
{
    return m_userViewPtr; // Changed m_userView to m_userViewPtr
}

void UserModeModule::setUserViewInstance(UserView* view)
{
    m_userViewPtr = view; // Changed m_userView to m_userViewPtr
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        // Disconnect any existing connection to avoid duplicates if called multiple times
        disconnect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested); // Changed m_userView to m_userViewPtr
        // Connect the new view instance
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested); // Changed m_userView to m_userViewPtr
        qInfo() << "UserModeModule: UserView instance set and connected.";
    } else {
        qWarning() << "UserModeModule: setUserViewInstance called with a null view.";
    }
}

void UserModeModule::loadConfiguration()
{
    m_whitelistedApps.clear(); // Use m_whitelistedApps
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.json";
    QFile configFile(configPath);

    if (!QFileInfo::exists(configPath)) {
        qWarning() << "Config file not found at" << configPath << "Creating a default one.";
        // Simplified: create default or copy from resources if available
        // For now, just warn and proceed with empty whitelist
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
            AppInfo app; // Use AppInfo
            app.name = appObj["name"].toString();
            app.path = appObj["path"].toString();
            app.mainExecutableHint = appObj["mainExecutableHint"].toString(); // Read the new field
            qDebug() << "UserModeModule::loadConfiguration - Loaded app:" << app.name << "Path:" << app.path << "Hint:" << app.mainExecutableHint;

            if (m_systemInteractionModulePtr) { // Use m_systemInteractionModulePtr
                app.icon = m_systemInteractionModulePtr->getIconForExecutable(app.path);
            } else {
                qWarning() << "SystemInteractionModule is null, cannot fetch icon for" << app.name;
                // Optionally load a default icon or icon from appObj if specified
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

            m_whitelistedApps.append(app); // Use m_whitelistedApps
        }
    }
    qInfo() << "Configuration loaded," << m_whitelistedApps.count() << "apps in whitelist.";
}


void UserModeModule::loadAndSetWhitelist()
{
    loadConfiguration(); // Reloads m_whitelistedApps from config
    if (m_userViewPtr) {
        qInfo() << "UserModeModule: Setting app list for UserView with" << m_whitelistedApps.count() << "apps.";
        m_userViewPtr->setAppList(m_whitelistedApps); // Use m_whitelistedApps
    } else {
        qWarning() << "UserModeModule: UserView is not available to set app list.";
    }
}

void UserModeModule::terminateActiveProcesses() {
    qInfo() << "UserModeModule: Terminating all active/launched processes.";
    for (QProcess* process : m_launchedProcesses.values()) { // Iterate over values of QHash
        if (process && process->state() != QProcess::NotRunning) {
            QString appPath = m_launchedProcesses.key(process); // Get key for this process
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
    // After attempting to terminate, clear the tracking map.
    // Note: QProcess objects that are children of UserModeModule will be deleted when UserModeModule is deleted.
    // If they are not children, they should be deleteLater'd here or upon their finished signal.
    // The current connect for finished() does deleteLater().
    m_launchedProcesses.clear();
    qInfo() << "Cleared tracked processes.";
}

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
        
        // Removed lambda callback, result will be handled by onApplicationActivated/Failed slots
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
                // Removed lambda callback, result will be handled by onApplicationActivated/Failed slots
                m_systemInteractionModulePtr->monitorAndActivateApplication(appPath, 
                                                                          0, // Launcher PID is 0 for re-activation of already known app
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

void UserModeModule::onApplicationActivated(const QString& appPath) {
    qInfo() << "UserModeModule::onApplicationActivated - Application" << appPath << "has been activated.";
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activated):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false); // Corrected call
    }
}

void UserModeModule::onApplicationActivationFailed(const QString& appPath) {
    qWarning() << "UserModeModule::onApplicationActivationFailed - Activation failed for" << appPath;
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activation failed):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->setAppLoadingState(appPath, false); // Corrected call
    }
}

void UserModeModule::onProcessStarted(const QString& appPath) {
    qDebug() << "UserModeModule::onProcessStarted (Restored) - Process started for:" << appPath;
}

void UserModeModule::onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "UserModeModule::onProcessFinished (Legacy Slot) - Process for" << appPath
              << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
    if (m_userViewPtr) { 
        m_userViewPtr->setAppLoadingState(appPath, false); // Corrected call
    }
}

void UserModeModule::onProcessError(const QString& appPath, QProcess::ProcessError error) {
    qWarning() << "UserModeModule::onProcessError (Legacy Slot) - Error for process" << appPath << "Error:" << error;
    if (m_userViewPtr) { 
        m_userViewPtr->setAppLoadingState(appPath, false); // Corrected call
    }
}

// Added definition for onProcessStateChanged to resolve LNK2019
void UserModeModule::onProcessStateChanged(QProcess::ProcessState newState) {
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    // To get appPath, we need to iterate m_launchedProcesses because QHash key is appPath
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

// --- Helper methods (potentially needed for advanced monitoring or if config changes) ---

bool UserModeModule::isAppWhitelisted(const QString& executablePath) const
{
    for (const auto& app : m_whitelistedApps) { // Use m_whitelistedApps
        if (QFileInfo(app.path).fileName().compare(QFileInfo(executablePath).fileName(), Qt::CaseInsensitive) == 0 ||
            app.path.compare(executablePath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QString UserModeModule::getAppPathForName(const QString& appName) const
{
    for (const auto& app : m_whitelistedApps) { // Use m_whitelistedApps
        if (app.name.compare(appName, Qt::CaseInsensitive) == 0) {
            return app.path;
        }
    }
    return QString();
}


void UserModeModule::startProcessMonitoringTimer() {
    if (!m_processMonitoringTimer) {
        m_processMonitoringTimer = new QTimer(this);
    }
    if (!m_processMonitoringTimer->isActive()) {
        connect(m_processMonitoringTimer, &QTimer::timeout, this, &UserModeModule::monitorLaunchedProcesses);
        m_processMonitoringTimer->start(15000); // e.g., check every 15 seconds
        qInfo() << "Process monitoring timer started.";
    }
}

void UserModeModule::monitorLaunchedProcesses() {
    auto it = m_launchedProcesses.begin();
    while (it != m_launchedProcesses.end()) {
        QProcess* process = it.value(); // Get value (QProcess*)
        QString appPath = it.key();   // Get key (QString)
        if (!process || process->state() == QProcess::NotRunning) {
            qInfo() << "Monitored process" << appPath << "is no longer running or invalid. Removing.";
            it = m_launchedProcesses.erase(it); 
            if(process) process->deleteLater();
        } else {
            ++it;
        }
    }
    if (m_launchedProcesses.isEmpty() && m_processMonitoringTimer && m_processMonitoringTimer->isActive()){
        // m_processMonitoringTimer->stop(); // Optionally stop if nothing to monitor
    }
}

QString UserModeModule::findAppPathForProcess(QProcess* process) {
    // Iterate to find the key for a given QProcess* value
    for(auto it = m_launchedProcesses.constBegin(); it != m_launchedProcesses.constEnd(); ++it) {
        if (it.value() == process) {
            return it.key();
        }
    }
    return QString();
}

void UserModeModule::updateUserAppList(const QList<AppInfo>& apps) {
    qDebug() << "UserModeModule::updateUserAppList - Updating apps from provided list. Count:" << apps.count();
    m_whitelistedApps = apps; // Update internal cache

    if (m_userViewPtr) {
        m_userViewPtr->setAppList(m_whitelistedApps); // Update UserView
        qDebug() << "UserModeModule::updateUserAppList - UserView updated with new app list.";
    } else {
        qWarning() << "UserModeModule::updateUserAppList - UserView pointer (m_userViewPtr) is null, cannot update view.";
    }
    // After updating from a direct list, we might consider the config "loaded" or synced for this session component.
    m_configLoaded = true; 
} 