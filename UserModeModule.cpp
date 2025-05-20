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
      m_processMonitoringTimer(new QTimer(this))
{
    qInfo() << "UserModeModule initialized.";
    loadConfiguration(); // Load initial whitelist from config

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

    for (auto it = m_launchedProcesses.constBegin(); it != m_launchedProcesses.constEnd(); ++it) {
        if (it.value() == appPath && it.key() && it.key()->state() != QProcess::NotRunning) {
            qInfo() << "Application" << appPath << "is already running. Attempting to bring to front.";
            if (m_systemInteractionModulePtr) { 
                // Use findMainWindowForProcessOrChildren as findMainWindowForProcessId might not exist or have different params
                HWND hwnd_raw = m_systemInteractionModulePtr->findMainWindowForProcessOrChildren(it.key()->processId(), QFileInfo(appPath).fileName());
                if (hwnd_raw) {
                    WId wid = reinterpret_cast<WId>(hwnd_raw); // Or static_cast<WId>(hwnd_raw) - HWND to WId
                    m_systemInteractionModulePtr->bringToFrontAndActivate(wid); 
                } else {
                    qWarning() << "Could not find window for already running process:" << appPath;
                }
            }
            return;
        }
    }

    QProcess *process = new QProcess(this);

    // Store the appPath before it's captured by the lambda
    // This is useful if you need the original path for identification
    // m_launchedProcesses.insert(process, appPath);

    connect(process, &QProcess::started, this, [this, process, appPath]() {
        qInfo() << "Process for" << appPath << "started. PID:" << process->processId();
        m_launchedProcesses.insert(process, appPath); // Insert after successful start

        if (m_systemInteractionModulePtr) { // Use m_systemInteractionModulePtr
            // Attempt to find and bring the main window to front
            // Adding a small delay as window might not be immediately available
            QTimer::singleShot(500, this, [this, process, appPath]() {
                if (!process || process->state() == QProcess::NotRunning) return;
                HWND hwnd_raw = m_systemInteractionModulePtr->findMainWindowForProcessOrChildren(process->processId(), QFileInfo(appPath).fileName());
                if (hwnd_raw) {
                    qInfo() << "Found window for" << appPath << "- bringing to front.";
                    WId wid = reinterpret_cast<WId>(hwnd_raw); // Or static_cast<WId>(hwnd_raw)
                    m_systemInteractionModulePtr->bringToFrontAndActivate(wid);
                } else {
                    qWarning() << "Could not find main window for" << appPath << "after delay. PID:" << process->processId();
                }
            });
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, appPath](int exitCode, QProcess::ExitStatus exitStatus) {
        qInfo() << "Process for" << appPath << "finished. Exit code:" << exitCode << "Status:" << exitStatus;
        m_launchedProcesses.remove(process);
        process->deleteLater(); // Important: schedule QProcess object for deletion
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, appPath](QProcess::ProcessError error) {
        qWarning() << "Error occurred for process" << appPath << ":" << error;
        emit applicationFailedToLaunch(QFileInfo(appPath).fileName(), process->errorString());
        m_launchedProcesses.remove(process); // Remove on error too
        process->deleteLater();
    });
    
    qInfo() << "Starting process:" << appPath;
    process->setProgram(appPath);
    process->start();

    if (!process->waitForStarted(3000)) { // Wait up to 3 seconds for start
        qWarning() << "Process" << appPath << "failed to start or timed out:" << process->errorString();
        emit applicationFailedToLaunch(QFileInfo(appPath).fileName(), process->errorString());
        m_launchedProcesses.remove(process); // Ensure removal if start fails
        delete process; // Delete immediately if waitForStarted fails and it wasn't cleaned up
        return;
    }
}


void UserModeModule::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString appPath = m_launchedProcesses.value(process, QString());
    qInfo() << "Process" << (appPath.isEmpty() ? "unknown" : appPath)
            << "finished. Exit code:" << exitCode << "Status:" << exitStatus;

    m_launchedProcesses.remove(process);
    process->deleteLater(); // Schedule for deletion
}

void UserModeModule::onProcessErrorOccurred(QProcess::ProcessError error)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString appPath = m_launchedProcesses.value(process, QString());
    qWarning() << "Error for process" << (appPath.isEmpty() ? "unknown" : appPath) << ":" << error << process->errorString();

    m_launchedProcesses.remove(process);
    process->deleteLater(); // Schedule for deletion
}

void UserModeModule::onProcessStateChanged(QProcess::ProcessState newState)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;
    QString appPath = m_launchedProcesses.value(process, QString());
    // qInfo() << "Process" << (appPath.isEmpty() ? "unknown" : appPath) << "state changed to:" << newState;
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