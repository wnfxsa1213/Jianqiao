#ifndef USERMODEMODULE_H
#define USERMODEMODULE_H

#include "common_types.h"
#include <QObject>
#include <QList>
#include <QMap> // For QMap
#include <QProcess>
#include <QTimer>
#include "UserView.h"       // For m_userView interaction
#include "SystemInteractionModule.h" // For icon fetching and process interaction
#include "JianqiaoCoreShell.h"
#include <QSet> // <<< ADD THIS LINE

// struct WhitelistedApp; // No longer needed, full definition in common_types.h
class UserView; // Forward declaration
class SystemInteractionModule; // Forward declaration for the pointer

class UserModeModule : public QObject
{
    Q_OBJECT

public:
    // SystemInteractionModule might be needed later for window management of launched apps
    explicit UserModeModule(JianqiaoCoreShell *coreShell, UserView* userView, SystemInteractionModule* systemInteraction, QObject *parent = nullptr);
    ~UserModeModule();

    void activate();
    void deactivate();
    void loadAndSetWhitelist(); // Only one declaration

    void showUserView(); // Called by CoreShell to activate this mode
    void hideUserView(); // Called by CoreShell to deactivate this mode
    bool isUserViewVisible() const;
    QWidget* getUserViewWidget(); // Returns the UserView widget
    UserView* getViewInstance(); // Returns UserView instance directly

    void setUserViewInstance(UserView* view); // Added setter

    void loadConfiguration(); // Ensure this populates m_whitelistedApps as QList<AppInfo>
    bool isAppWhitelisted(const QString& processName) const;
    QString getAppPathForName(const QString& appName) const;
    void updateUserAppList(const QList<AppInfo>& apps); // New method
    void terminateActiveProcesses(); // Added declaration

signals:
    void userModeActivated();
    void userModeDeactivated();
    void applicationFailedToLaunch(const QString& appName, const QString& error); // Consider using this
    void userAppListUpdated(const QList<AppInfo>& apps); // Added signal declaration

public slots:
    // Changed signature to match cpp
    void onApplicationLaunchRequested(const QString& appPath, const QString& appName);
    void onProcessStateChanged(QProcess::ProcessState newState);
    void onApplicationActivated(const QString& appPath);
    void onApplicationActivationFailed(const QString& appPath);

    // Corrected slot declarations to match implementations that take appPath
    void onProcessStarted(const QString& appPath);
    void onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(const QString& appPath, QProcess::ProcessError error);

private:
    // Changed signature to match cpp
    void launchApplication(const QString& appPath, const QString& appName);
    void monitorLaunchedProcess(QProcess* process, const QString& appName);
    QString findExecutableName(const QString& appPath) const;
    void startProcessMonitoringTimer();
    void monitorLaunchedProcesses();
    QString findAppPathForProcess(QProcess* process);

    JianqiaoCoreShell *m_coreShellPtr;
    UserView *m_userViewPtr; // Renamed to m_userViewPtr
    SystemInteractionModule *m_systemInteractionModulePtr; // Renamed to m_systemInteractionModulePtr
    QList<AppInfo> m_whitelistedApps;
    // QMap<QProcess*, QString> m_launchedProcesses; // Changed to QHash<QString, QProcess*> in cpp. Let's use QHash in header too for consistency if cpp is the newer version
    QHash<QString, QProcess*> m_launchedProcesses; // Matched type from latest cpp
    QTimer* m_processMonitoringTimer;
    bool m_configLoaded = false; // ADDED

    // QPointer<SystemInteractionModule> m_systemInteractionModule; // Using QPointer for safety
    // bool m_isActive; // Flag to indicate if user mode is currently active

    QString m_configFilePath;
    // QMap<QString, QProcess*> m_runningProcesses; // This seems duplicate with m_launchedProcesses, check usage
    QMap<QString, QTimer*> m_launchTimers;      // Timers for launch timeout
    QSet<QString> m_launchingApps;          // Tracks apps currently in the process of launching
    QSet<QString> m_pendingActivationApps; // <<< ADD THIS LINE

    // QList<AppInfo> m_currentApps; // To store the list of apps - replaced by m_whitelistedApps if that is the main store
};

#endif // USERMODEMODULE_H 