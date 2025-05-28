#ifndef SYSTEMINTERACTIONMODULE_H
#define SYSTEMINTERACTIONMODULE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QMap>
#include <Windows.h> // Required for HHOOK and KBDLLHOOKSTRUCT
#include <QWidget> // Added to ensure WId is defined
#include <QVector>
#include <QSet> // For m_pressedKeys
#include <QIcon> // Added for getIconForExecutable
#include <QTimer> // ADDED for monitoring
#include <QJsonObject>
#include <QAbstractNativeEventFilter>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QWindow>
#include <QScreen>
#include <QPixmap>
#include <QFileIconProvider>
#include <QDebug>
#include <memory> // Keep for now, might be used elsewhere or for comparison
#include "common_types.h" // Ensure SuggestedWindowHints is known

#if defined(Q_OS_WIN)
#include <windows.h> // For HWND, DWORD, etc.
#endif

// Helper structure for monitoring applications launched via a launcher
struct MonitoringInfo {
    QString originalLauncherPath;
    QString mainExecutableHint;
    QString windowHintsJson; // Storing as JSON string
    int attempts;
    quint32 launcherPid;
    bool forceActivateOnly;
    QTimer* timer; 

    MonitoringInfo() : attempts(0), launcherPid(0), forceActivateOnly(false), timer(nullptr) {}

    // Rule of Five no longer needed as we will manage this via raw pointers in the map
    // // 1. Destructor 
    // ~MonitoringInfo() { /* if (timer) { timer->stop(); delete timer; } // Only if MonitoringInfo truly owned it */ }
    // // 2. Copy constructor - delete or manage carefully if pointers are involved
    // MonitoringInfo(const MonitoringInfo& other) = delete; // Simplest if copies are not needed
    // // 3. Copy assignment operator - delete or manage carefully
    // MonitoringInfo& operator=(const MonitoringInfo& other) = delete;
    // // 4. Move constructor - manage carefully if pointers are involved
    // MonitoringInfo(MonitoringInfo&& other) noexcept = delete;
    // // 5. Move assignment operator - manage carefully if pointers are involved
    // MonitoringInfo& operator=(MonitoringInfo&& other) noexcept = delete;
};

class SystemInteractionModule : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit SystemInteractionModule(QObject *parent = nullptr);
    ~SystemInteractionModule();

    bool installKeyboardHook();
    void uninstallKeyboardHook();
    bool loadConfiguration();
    void bringToFrontAndActivate(WId windowId);
    HWND findMainWindowForProcess(DWORD processId, const QJsonObject& windowHints = QJsonObject());
    QPair<HWND, int> findMainWindowForProcessWithScore(DWORD processId, const QJsonObject& windowHints = QJsonObject());
    HWND findMainWindowForProcessOrChildren(DWORD initialPid, const QString& executableNameHint);
    void monitorAndActivateApplication(
        const QString& originalAppPath,
        quint32 launcherPid,
        const QString& mainExecutableHint = QString(),
        const QJsonObject& windowHints = QJsonObject(),
        bool forceActivateOnly = false
    );
    void setUserModeActive(bool active);
    bool isUserModeActive() const;
    DWORD stringToVkCode(const QString& keyString);
    QString vkCodeToString(DWORD vkCode) const;
    QIcon getIconForExecutable(const QString& executablePath);
    QStringList getCurrentAdminLoginHotkeyStrings() const;
    bool isMonitoring(const QString& appPath) const;
    void stopMonitoringProcess(const QString& appPath);

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void adminLoginRequested();
    void applicationActivated(const QString& appPath);
    void applicationActivationFailed(const QString& appPath, const QString& reason);
    void detectionCompleted(const SuggestedWindowHints& hints, bool success, const QString& errorString);

public slots:
    void installHookAsync();
    void uninstallHookAsync();
    void startExecutableDetection(const QString& executablePath, const QString& appName);

// private slots section
private slots:
    void onMonitoringTimerTimeout();

// private members and methods (non-slots)
private:
    static HHOOK keyboardHook_;
    static SystemInteractionModule* instance_; // For emitting signals from static callback
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static QMap<QString, DWORD> initializeVkCodeMap();
    static const QMap<QString, DWORD> VK_CODE_MAP;
    static BOOL CALLBACK EnumWindowsProcWithHints(HWND hwnd, LPARAM lParam); // Moved static callback here

    // Regular private methods
    SuggestedWindowHints performExecutableDetectionLogic(const QString& executablePath, const QString& initialAppName);
    bool isKeyInteresting(DWORD vkCode, bool isKeyDown);
    void updateCurrentHotkeyState(DWORD vkCode, bool isKeyDown);
    bool checkAdminLoginHotkey();
    void saveConfiguration();
    bool isProcessRunning(DWORD pid);
    QList<DWORD> findChildProcesses(DWORD parentPid);
    QString getProcessNameByPid(DWORD pid);
    DWORD findProcessIdByName(const QString& processName);
    QDateTime getProcessCreationTime(DWORD processId);
    QList<DWORD> getAllProcessIds();
    bool isModifierKey(DWORD vkCode) const;
    void activateWindow(HWND hwnd); // Moved here

    // Private member variables
    QList<DWORD> m_adminLoginHotkey;
    QString m_configPath;
    bool m_userModeActive;
    bool m_isHookInstalled;
    QSet<DWORD> m_pressedKeys;
    QSet<DWORD> m_userModeBlockedVkCodes;
    QMap<QString, MonitoringInfo*> m_monitoringApps;
    QList<DWORD> m_adminLoginHotkeySequence; // Now clearly in private section
    int HINT_DETECTION_DELAY_MS; // 探测等待时间（毫秒），支持动态配置

};

#endif // SYSTEMINTERACTIONMODULE_H 