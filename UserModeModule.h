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

// struct WhitelistedApp; // No longer needed, full definition in common_types.h
class UserView; // Forward declaration
class SystemInteractionModule; // Forward declaration for the pointer

class UserModeModule : public QObject
{
    Q_OBJECT

public:
    // SystemInteractionModule might be needed later for window management of launched apps
    explicit UserModeModule(UserView* userView, SystemInteractionModule* systemInteractionModule, QObject *parent = nullptr);
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

signals:
    void userModeActivated();
    void userModeDeactivated();
    void applicationFailedToLaunch(const QString& appName, const QString& error); // Consider using this

public slots:
    void onApplicationLaunchRequested(const QString& appPath);
    void onProcessStateChanged(QProcess::ProcessState newState);
    void onApplicationActivated(const QString& appPath);

    // Corrected slot declarations to match implementations that take appPath
    void onProcessStarted(const QString& appPath);
    void onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(const QString& appPath, QProcess::ProcessError error);

private:
    void launchApplication(const QString& appPath, const QString& appName = QString());
    void monitorLaunchedProcess(QProcess* process, const QString& appName);
    QString findExecutableName(const QString& appPath) const;
    void startProcessMonitoringTimer();
    void monitorLaunchedProcesses();
    QString findAppPathForProcess(QProcess* process);

    UserView* m_userView; // UI for user mode
    QList<AppInfo> m_whitelistedApps; // Changed from m_currentWhitelist
    QMap<QProcess*, QString> m_launchedProcesses; // Stores launched QProcess and its original app path
    QTimer* m_processMonitoringTimer;
    SystemInteractionModule* m_systemInteractionModulePtr; // Added declaration
    bool m_configLoaded = false; // ADDED

    // QPointer<SystemInteractionModule> m_systemInteractionModule; // Using QPointer for safety
    // bool m_isActive; // Flag to indicate if user mode is currently active

    QString m_configFilePath;
};

#endif // USERMODEMODULE_H 