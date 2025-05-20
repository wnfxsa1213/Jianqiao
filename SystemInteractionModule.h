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
    void setUserModeActive(bool active);
    bool isUserModeActive() const;
    DWORD stringToVkCode(const QString& keyString);
    QString vkCodeToString(DWORD vkCode) const;
    QIcon getIconForExecutable(const QString& executablePath);
    QStringList getCurrentAdminLoginHotkeyStrings() const;

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

    static QMap<QString, DWORD> initializeVkCodeMap();
    static const QMap<QString, DWORD> VK_CODE_MAP;
    bool isModifierKey(DWORD vkCode) const; // Helper to check if a key is a common modifier

signals:
    void keyPressed(DWORD vkCode);
    void adminLoginRequested();

};

#endif // SYSTEMINTERACTIONMODULE_H 