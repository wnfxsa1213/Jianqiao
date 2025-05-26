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
UserModeModule::UserModeModule(UserView* userView, SystemInteractionModule* systemInteractionModule, QObject *parent)
    : QObject(parent),
      m_userView(userView),
      m_systemInteractionModulePtr(systemInteractionModule), // Initialize m_systemInteractionModulePtr
      m_processMonitoringTimer(new QTimer(this)),
      m_configLoaded(false)
{
    if (!m_systemInteractionModulePtr) {
        qCritical() << "UserModeModule: SystemInteractionModule pointer is null!";
    }
    if (!m_userView) {
        qCritical() << "UserModeModule: UserView pointer is null!";
    }

    loadConfiguration();
    if (m_userView) {
        connect(m_userView, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        qInfo() << "UserModeModule initialized.";
    } else {
        qWarning() << "UserModeModule: UserView is null, cannot connect signals.";
    }

    // Connect to SystemInteractionModule's activation signal
    if (m_systemInteractionModulePtr) {
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::applicationActivated, this, &UserModeModule::onApplicationActivated);
    }

    // Connect signals from UserView
    if (m_userView) {
        connect(m_userView, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
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
    if(m_userView) {
        m_userView->show();
        m_userView->update();      // Force a repaint
        m_userView->adjustSize();  // Adjust size if necessary due to new content
    }
    emit userModeActivated();
}

void UserModeModule::deactivate()
{
    qInfo() << "User mode deactivated.";
    if(m_userView) {
        m_userView->hide();
    }
    // Optionally, could terminate whitelisted apps here, or leave them running
    emit userModeDeactivated();
}

void UserModeModule::showUserView()
{
    if (m_userView) {
        qDebug() << "[UserModeModule::showUserView] Preparing to show UserView. Whitelisted app count in UserModeModule:" << m_whitelistedApps.count();
        m_userView->setAppList(m_whitelistedApps);

        m_userView->show();
        qInfo() << "UserModeModule: UserView shown.";
    } else {
        qWarning() << "UserModeModule::showUserView: m_userView is null!";
    }
}

void UserModeModule::hideUserView()
{
    if (m_userView) {
        m_userView->hide();
        qInfo() << "UserModeModule: UserView hidden.";
    } else {
        qWarning() << "UserModeModule::hideUserView: m_userView is null!";
    }
}

bool UserModeModule::isUserViewVisible() const
{
    if (m_userView) {
        return m_userView->isVisible();
    }
    qWarning() << "UserModeModule::isUserViewVisible: m_userView is null!";
    return false;
}

QWidget* UserModeModule::getUserViewWidget()
{
    return m_userView;
}

UserView* UserModeModule::getViewInstance()
{
    return m_userView;
}

void UserModeModule::setUserViewInstance(UserView* view)
{
    m_userView = view;
    if (m_userView) {
        // Disconnect any existing connection to avoid duplicates if called multiple times
        disconnect(m_userView, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
        // Connect the new view instance
        connect(m_userView, &UserView::applicationLaunchRequested, this, &UserModeModule::onApplicationLaunchRequested);
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
    if (m_userView) {
        qInfo() << "UserModeModule: Setting app list for UserView with" << m_whitelistedApps.count() << "apps.";
        m_userView->setAppList(m_whitelistedApps); // Use m_whitelistedApps
    } else {
        qWarning() << "UserModeModule: UserView is not available to set app list.";
    }
}


void UserModeModule::onApplicationLaunchRequested(const QString& appPath)
{
    qInfo() << "Application launch requested for path:" << appPath;

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
        if (m_userView) {
            m_userView->resetAppIconState(appPath); // Reset icon if launch fails early
        }
        return;
    }

    // Check if a QProcess object already exists and is running for this appPath
    for (QProcess* existingProcess : m_launchedProcesses.keys()) {
        if (m_launchedProcesses.value(existingProcess) == appPath && existingProcess->state() != QProcess::NotRunning) {
            qInfo() << "Application" << appPath << "is already running (PID:" << existingProcess->processId() << "). Attempting to activate.";
            if (m_userView) {
                // Optional: Indicate to user view that we are trying to re-activate
                // m_userView->setAppIconLaunching(appPath, true); // Or similar
            }
            if (m_systemInteractionModulePtr) {
                m_systemInteractionModulePtr->monitorAndActivateApplication(appPath, existingProcess->processId(), targetAppInfo.mainExecutableHint, true); // forceActivateOnly = true
            } else {
                 if (m_userView) m_userView->resetAppIconState(appPath);
            }
            return;
        }
    }
    
    // Clean up any old QProcess object for this appPath if it's NotRunning
    QList<QProcess*> processesToRemove;
    for (QProcess* p : m_launchedProcesses.keys()) {
        if (m_launchedProcesses.value(p) == appPath) { // Check if it's for the same app
            if (p->state() == QProcess::NotRunning) {
                processesToRemove.append(p);
            }
        }
    }
    for (QProcess* p : processesToRemove) {
        qDebug() << "UserModeModule: Cleaning up old, not running QProcess for" << appPath;
        m_launchedProcesses.remove(p);
        p->deleteLater();
    }


    QProcess* process = new QProcess(this);
    // Store appPath with the process for later identification in finished/error signals
    // m_launchedProcesses.insert(process, appPath); // Moved to QProcess::started

    // Capture appPath for lambdas
    QString capturedAppPath = appPath;
    AppInfo capturedAppInfo = targetAppInfo;

    connect(process, &QProcess::started, this, [this, process, capturedAppPath, capturedAppInfo]() {
        qInfo() << "[STARTED LAMBDA] Launcher process for" << capturedAppPath << "started. PID:" << process->processId();
        
        qDebug() << "[STARTED LAMBDA] Inserting into m_launchedProcesses.";
        m_launchedProcesses.insert(process, capturedAppPath); 
        qDebug() << "[STARTED LAMBDA] Successfully inserted into m_launchedProcesses. Map size:" << m_launchedProcesses.size();

        if (m_systemInteractionModulePtr) {
            qDebug() << "[STARTED LAMBDA] m_systemInteractionModulePtr is VALID. Calling monitorAndActivateApplication.";
            m_systemInteractionModulePtr->monitorAndActivateApplication(capturedAppPath, process->processId(), capturedAppInfo.mainExecutableHint, false); // forceActivateOnly = false
             qDebug() << "[STARTED LAMBDA] monitorAndActivateApplication call made for" << capturedAppPath;
        } else {
            qWarning() << "[STARTED LAMBDA] m_systemInteractionModulePtr is NULL. Cannot monitor or activate window for" << capturedAppPath;
            // If system interaction is not available, we can't do much more.
            // UserView's timer will eventually reset the icon.
        }
        qDebug() << "[STARTED LAMBDA] Finished execution of started lambda for" << capturedAppPath;
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process, capturedAppPath](int exitCode, QProcess::ExitStatus exitStatus) {
        qInfo() << "Process for (launcher)" << capturedAppPath << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
        // Note: Launcher finishing doesn't mean the main app is closed.
        // The main app's lifecycle is now primarily handled by SystemInteractionModule's polling (if hint was provided)
        // or UserView's timeout.
        // We only reset the icon here IF the SystemInteractionModule is NOT actively monitoring
        // OR if the hint was empty (meaning monitoring never started via polling).
        
        bool wasMonitoring = false;
        if(m_systemInteractionModulePtr){
            // A bit indirect, but if a timer exists, it means robust monitoring was active or attempted.
             wasMonitoring = m_systemInteractionModulePtr->isMonitoring(capturedAppPath);
        }

        AppInfo appInfo;
        bool appInfoFound = false;
        for(const auto& currentApp : m_whitelistedApps){
            if(currentApp.path == capturedAppPath){
                appInfo = currentApp;
                appInfoFound = true;
                break;
            }
        }

        if (!appInfoFound || appInfo.mainExecutableHint.isEmpty() || !wasMonitoring) {
             qDebug() << "UserModeModule::onProcessFinished - Launcher for" << capturedAppPath << "ended. No robust monitoring was active (no hint or not monitoring). Resetting icon via UserView.";
             if (m_userView) {
                m_userView->resetAppIconState(capturedAppPath); // Also stops UserView's internal timer
             }
        } else {
            qDebug() << "UserModeModule::onProcessFinished - Launcher for" << capturedAppPath << "ended. Robust monitoring is/was active via SystemInteractionModule. Icon reset will be handled by onApplicationActivated or UserView timeout.";
        }

        m_launchedProcesses.remove(process);
        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, capturedAppPath](QProcess::ProcessError error) {
        qWarning() << "Error launching process for" << capturedAppPath << "Error:" << error;
        if (m_userView) {
            m_userView->resetAppIconState(capturedAppPath); // Also stops UserView's internal timer
        }
        m_launchedProcesses.remove(process);
        process->deleteLater();
    });

    qInfo() << "Starting launcher process:" << appPath;
    if (m_userView) {
        m_userView->setAppIconLaunching(appPath, true); // Set icon to launching state and start internal timer
    }
    process->start(appPath);
}

void UserModeModule::onApplicationActivated(const QString& appPath) {
    qInfo() << "UserModeModule::onApplicationActivated - Application" << appPath << "has been activated.";
    if (m_userView) {
        m_userView->resetAppIconState(appPath); // This will set icon to normal and stop UserView's internal timer.
    }
    // No need to interact with QProcess objects here, their lifecycle is separate (launcher vs main app)
    // SystemInteractionModule has finished its monitoring for this activation.
}

void UserModeModule::onProcessStarted(const QString& appPath) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    // but keeping it for now in case it's connected elsewhere or for future use.
    qDebug() << "UserModeModule::onProcessStarted (Legacy Slot) - Process started for:" << appPath;
}

void UserModeModule::onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    qWarning() << "UserModeModule::onProcessFinished (Legacy Slot) - Process for" << appPath
              << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
    // It's important that if this slot IS used, it should also reset the icon state.
    // However, the lambda is preferred.
    if (m_userView) {
        m_userView->resetAppIconState(appPath);
    }
}

void UserModeModule::onProcessError(const QString& appPath, QProcess::ProcessError error) {
    // This slot might be redundant now due to the lambda in onApplicationLaunchRequested
    qWarning() << "UserModeModule::onProcessError (Legacy Slot) - Error for process" << appPath << "Error:" << error;
    if (m_userView) {
        m_userView->resetAppIconState(appPath);
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