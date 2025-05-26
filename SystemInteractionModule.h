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

#if defined(Q_OS_WIN)
#include <windows.h> // For HWND, DWORD, etc.
#endif

class SystemInteractionModule : public QObject
{
    Q_OBJECT
public:
    explicit SystemInteractionModule(QObject *parent = nullptr);
    ~SystemInteractionModule();

    bool installKeyboardHook();
    void uninstallKeyboardHook();
    bool loadConfiguration();
    void bringToFrontAndActivate(WId windowId);
    HWND findMainWindowForProcess(DWORD processId);
    HWND findMainWindowForProcessOrChildren(DWORD initialPid, const QString& executableNameHint);
    void monitorAndActivateApplication(const QString& originalAppPath, qint64 launcherPid, const QString& mainExecutableHint, bool forceActivateOnly = false);
    void setUserModeActive(bool active);
    bool isUserModeActive() const;
    DWORD stringToVkCode(const QString& keyString);
    QString vkCodeToString(DWORD vkCode) const;
    QIcon getIconForExecutable(const QString& executablePath);
    QStringList getCurrentAdminLoginHotkeyStrings() const;
    bool isMonitoring(const QString& appPath) const;

private:
    static HHOOK keyboardHook_;
    static SystemInteractionModule* instance_; // For emitting signals from static callback
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    QList<DWORD> m_adminLoginHotkey;
    QString m_configPath;
    bool m_userModeActive; // To track if user mode is active
    bool m_isHookInstalled; // Added declaration
    QSet<DWORD> m_pressedKeys; // Tracks all currently pressed keys
    QSet<DWORD> m_userModeBlockedVkCodes; // Added: Keys to block in user mode

    QMap<QString, QTimer*> m_monitoringTimers; // Timers for monitorAndActivateApplication, key: originalAppPath
    QMap<QString, int> m_monitoringAttempts;   // Attempts for monitorAndActivateApplication, key: originalAppPath

    static QMap<QString, DWORD> initializeVkCodeMap();
    static const QMap<QString, DWORD> VK_CODE_MAP;
    bool isModifierKey(DWORD vkCode) const; // Helper to check if a key is a common modifier

    DWORD findProcessIdByName(const QString& executableName); // New private helper
    void attemptMainExecutableDetection(DWORD launcherPid, QString launcherPath); // New private slot

    QSet<QString> m_hintSuggestionAttemptedPaths; // Added: Tracks paths for which hint suggestion was made

signals:
    void keyPressed(DWORD vkCode);
    void adminLoginRequested();
    void applicationActivated(const QString& originalAppPath);

};

#endif // SYSTEMINTERACTIONMODULE_H 