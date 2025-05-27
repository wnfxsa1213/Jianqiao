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
    if (m_userViewPtr) { // Changed m_userView to m_userViewPtr
        connect(m_userViewPtr, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested); // Changed m_userView to m_userViewPtr
        qInfo() << "UserModeModule initialized.";
    } else {
        qWarning() << "UserModeModule: UserView is null, cannot connect signals.";
    }

    // Connect to SystemInteractionModule's activation signal
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
    // Cleanup: Terminate any managed processes
    for (QProcess* process : m_launchedProcesses.keys()) {
        if (process && process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(1000)) { // Wait 1 sec
                process->kill();
            }
        }
        // delete process; // QProcess objects are QObjects and should be deleted via deleteLater or handled by parent
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


void UserModeModule::onApplicationLaunchRequested(const QString& appPath)
{
    qInfo() << "Application launch requested for path:" << appPath;

    if (m_pendingActivationApps.contains(appPath)) {
        qDebug() << "UserModeModule::onApplicationLaunchRequested - Application" << appPath << "is already pending activation. Ignoring duplicate request.";
        return;
    }

    AppInfo targetAppInfo;
    bool appInfoFound = false;
    for (const auto& app : m_whitelistedApps) {
        if (app.path == appPath) {
            targetAppInfo = app;
            appInfoFound = true;
            break;
        }
    }

    if (!appInfoFound) {
        qWarning() << "UserModeModule::onApplicationLaunchRequested - App path not found in whitelist:" << appPath;
        if (m_userViewPtr) {
            m_userViewPtr->resetAppIconState(appPath);
        }
        return;
    }

    qInfo() << "UserModeModule: Attempting to launch" << targetAppInfo.name << "from path:" << targetAppInfo.path << "MainExe Hint:" << targetAppInfo.mainExecutableHint;

    QProcess *process = new QProcess(this);
    m_launchedProcesses.insert(process, appPath); // Store the appPath with the process

    // Add to pending activation set before starting the process
    m_pendingActivationApps.insert(appPath);
    qDebug() << "UserModeModule: Added" << appPath << "to pending activation apps:" << m_pendingActivationApps;

    // Capture necessary variables for lambda
    QString capturedAppPath = appPath;
    QString capturedAppName = targetAppInfo.name;
    QString capturedMainHint = targetAppInfo.mainExecutableHint;
    QJsonObject capturedWindowHints = targetAppInfo.windowFindingHints;

    connect(process, &QProcess::started, this, [this, capturedAppPath, capturedAppName, capturedMainHint, capturedWindowHints, process]() {
        qInfo() << "UserModeModule: Process for" << capturedAppPath << "(" << capturedAppName << ") started successfully. PID:" << process->processId();
        // DO NOT remove from m_pendingActivationApps here. SystemInteractionModule will handle activation.
        if (this->m_userViewPtr) {
            this->m_userViewPtr->setAppIconLaunching(capturedAppPath, true);
        }
        if (this->m_systemInteractionModulePtr) {
            this->m_systemInteractionModulePtr->monitorAndActivateApplication(
                capturedAppPath,
                static_cast<quint32>(process->processId()),
                capturedMainHint,
                capturedWindowHints,
                false
            );
        } else {
            qWarning() << "UserModeModule: SystemInteractionModule is null, cannot monitor/activate" << capturedAppName;
            // If SystemInteractionModule is null, we can't monitor, so remove from pending and reset icon
            this->m_pendingActivationApps.remove(capturedAppPath);
             qDebug() << "UserModeModule: Removed" << capturedAppPath << "from pending activation apps (SystemInteractionModule was null):" << this->m_pendingActivationApps;
            if (this->m_userViewPtr) {
                this->m_userViewPtr->resetAppIconState(capturedAppPath);
            }
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this, process, capturedAppPath, capturedAppName, capturedMainHint]
        (int exitCode, QProcess::ExitStatus exitStatus) {
        
        qInfo() << "UserModeModule: Process for" << capturedAppPath << "(" << capturedAppName << ") finished. Exit code:" << exitCode << "Status:" << exitStatus;
        m_launchedProcesses.remove(process);

        // If an app has a mainExecutableHint, it means the process that just finished was likely a launcher.
        // The actual main application's activation state (and thus icon state) will be determined by
        // SystemInteractionModule emitting either applicationActivated or signals indicating failure/timeout.
        // If there's no mainExecutableHint, then the process that finished *was* the target application.
        if (capturedMainHint.isEmpty()) {
            qDebug() << "UserModeModule (finished):" << capturedAppName << "has no mainExecutableHint.";
            // Process finished, and it was the main target (no hint). Remove from pending.
            this->m_pendingActivationApps.remove(capturedAppPath);
            qDebug() << "UserModeModule: Removed" << capturedAppPath << "from pending activation apps (no hint, process finished):" << this->m_pendingActivationApps;
            if (exitStatus == QProcess::CrashExit) {
                qWarning() << "UserModeModule (finished): Monitored application" << capturedAppPath << "crashed.";
            }
            if (this->m_userViewPtr) {
                qDebug() << "UserModeModule (finished): Resetting icon for" << capturedAppPath;
                this->m_userViewPtr->resetAppIconState(capturedAppPath);
            }
        } else {
            qDebug() << "UserModeModule (finished): Launcher" << capturedAppPath << "for" << capturedMainHint << "finished. Main app monitoring continues via SystemInteractionModule.";
            // For launchers, DO NOT remove from m_pendingActivationApps here.
            // It will be removed on SystemInteractionModule::applicationActivated or a timeout/failure signal from it.
        }

        process->deleteLater();
        qDebug() << "UserModeModule (finished): QProcess for" << capturedAppPath << "marked for deletion.";
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, capturedAppPath, capturedAppName](QProcess::ProcessError error) {
        qWarning() << "UserModeModule: Error launching" << capturedAppName << "(" << capturedAppPath << ") Error:" << error << process->errorString();
        this->m_launchedProcesses.remove(process);
        this->m_pendingActivationApps.remove(capturedAppPath); // Error occurred, remove from pending.
        qDebug() << "UserModeModule: Removed" << capturedAppPath << "from pending activation apps (error occurred):" << this->m_pendingActivationApps;

        if (this->m_userViewPtr) {
            this->m_userViewPtr->resetAppIconState(capturedAppPath);
        }
        QMessageBox::critical(m_userViewPtr, "Launch Error", QString("Failed to launch %1: %2").arg(capturedAppName, process->errorString()));
        process->deleteLater();
    });

    process->setProgram(targetAppInfo.path);
    qDebug() << "UserModeModule: Starting process:" << targetAppInfo.path;
    process->start();
}

void UserModeModule::onApplicationActivated(const QString& appPath) {
    qInfo() << "UserModeModule::onApplicationActivated - Application" << appPath << "has been activated.";
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activated):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->resetAppIconState(appPath);
    }
}

void UserModeModule::onApplicationActivationFailed(const QString& appPath) {
    qWarning() << "UserModeModule::onApplicationActivationFailed - Activation failed for" << appPath;
    m_pendingActivationApps.remove(appPath);
    qDebug() << "UserModeModule: Removed" << appPath << "from pending activation apps (activation failed):" << m_pendingActivationApps;
    if (m_userViewPtr) {
        m_userViewPtr->resetAppIconState(appPath);
        // Optionally, show a message to the user in UserView or via QMessageBox
        // QMessageBox::warning(m_userViewPtr, "Activation Failed", QString("Could not activate the main window for %1.").arg(appPath));
    }
}

void UserModeModule::onProcessStarted(const QString& appPath) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    // but keeping it for now in case it's connected elsewhere or for future use.
    qDebug() << "UserModeModule::onProcessStarted (Restored) - Process started for:" << appPath;
}

void UserModeModule::onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    qWarning() << "UserModeModule::onProcessFinished (Legacy Slot) - Process for" << appPath
              << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
    // It's important that if this slot IS used, it should also reset the icon state.
    // However, the lambda is preferred.
    if (m_userViewPtr) { // Changed
        m_userViewPtr->resetAppIconState(appPath); // Changed
    }
}

void UserModeModule::onProcessError(const QString& appPath, QProcess::ProcessError error) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    qWarning() << "UserModeModule::onProcessError (Legacy Slot) - Error for process" << appPath << "Error:" << error;
    if (m_userViewPtr) { // Changed
        m_userViewPtr->resetAppIconState(appPath); // Changed
    }
}

// Added definition for onProcessStateChanged to resolve LNK2019
void UserModeModule::onProcessStateChanged(QProcess::ProcessState newState) {
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    QString appPath = m_launchedProcesses.value(process, QString()); // Try to get appPath
    qDebug() << "UserModeModule::onProcessStateChanged - Process for" << (appPath.isEmpty() ? "unknown app" : appPath)
             << "changed state to:" << newState;
    // Add any specific logic needed when a process state changes.
    // For example, you might want to handle QProcess::Starting or QProcess::Running states
    // if not already handled by QProcess::started() signal.
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
    // This is a basic example. Real monitoring might involve checking if windows are still valid,
    // or if processes have become unresponsive, etc.
    // qInfo() << "Monitoring launched processes...";
    auto it = m_launchedProcesses.begin();
    while (it != m_launchedProcesses.end()) {
        QProcess* process = it.key();
        QString appPath = it.value();
        if (!process || process->state() == QProcess::NotRunning) {
            qInfo() << "Monitored process" << appPath << "is no longer running or invalid. Removing.";
            it = m_launchedProcesses.erase(it); // Erase and get next iterator
            if(process) process->deleteLater();
        } else {
            // qInfo() << "Process" << appPath << "is still running.";
            ++it;
        }
    }
    if (m_launchedProcesses.isEmpty() && m_processMonitoringTimer->isActive()){
         // qInfo() << "No active processes to monitor. Stopping timer.";
         // m_processMonitoringTimer->stop(); // Optionally stop if nothing to monitor
    }
}

QString UserModeModule::findAppPathForProcess(QProcess* process) {
    return m_launchedProcesses.value(process, QString());
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