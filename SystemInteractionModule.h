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
#include "AppStatus.h" // 确保包含AppStatus定义
#include "common_types.h" // 假设AppInfo定义在此

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

// 新增：窗口候选信息结构体
struct WindowCandidateInfo {
    HWND hwnd;
    QString className;
    QString title;
    bool isVisible;
    bool isTopLevel;
    DWORD processId;
    int score;
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
    // 查找指定进程的主窗口（带分数，支持Hint打分，静态函数，便于递归调用）
    static QPair<HWND, int> findMainWindowForProcessWithScore(DWORD processId, const QJsonObject& windowHints = QJsonObject());
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

    // 新增：统一获取配置文件路径的静态函数声明
    static QString getConfigFilePath();

    /**
     * @brief 获取所有白名单应用的实时状态
     * @param whitelist 当前白名单应用信息列表
     * @return 所有应用的AppStatus状态列表
     */
    QList<AppStatus> getAllAppStatus(const QList<AppInfo>& whitelist);

    void activateWindow(HWND hwnd); // Moved here, now public

    /**
     * @brief 自动采集并推荐主窗口特征（windowFindingHints）
     * @param processId 目标进程ID
     * @return 推荐的windowFindingHints（包含primaryClassName、titleContains等）
     */
    static QJsonObject autoDetectWindowFindingHints(DWORD processId);

    /**
     * @brief 递归查找主窗口并收集所有分数大于0的候选窗口
     * @param processId 目标进程ID
     * @param windowHints 查找Hint
     * @param candidates 用于收集所有候选窗口信息
     * @param depth 当前递归深度
     * @param maxDepth 最大递归深度
     * @return 最优主窗口句柄及分数
     */
    static QPair<HWND, int> findMainWindowRecursiveWithCandidates(
        DWORD processId,
        const QJsonObject& windowHints,
        QList<WindowCandidateInfo>& candidates,
        int depth = 0,
        int maxDepth = 4);

    /**
     * @brief 获取最近一次探测的候选窗口信息
     * @return 候选窗口信息列表
     */
    QList<WindowCandidateInfo> getLastDetectionCandidates() const;

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
    QString vkCodesToString(const QList<DWORD>& codes) const; // 新增：辅助函数声明

    // Private member variables
    QList<DWORD> m_adminLoginHotkey;
    QString m_configPath;
    bool m_userModeActive;
    bool m_isHookInstalled;
    QSet<DWORD> m_pressedKeys;
    QSet<DWORD> m_userModeBlockedVkCodes;
    QList<QList<DWORD>> m_userModeBlockedKeyCombinations;
    QMap<QString, MonitoringInfo*> m_monitoringApps;
    QList<DWORD> m_adminLoginHotkeySequence; // Now clearly in private section
    int HINT_DETECTION_DELAY_MS; // 探测等待时间（毫秒），支持动态配置
    QString m_lastActivatedAppPath; // 新增：记录最近一次被激活的应用路径

    // 新增：递归查找主窗口（支持特殊类型应用）
    static QPair<HWND, int> findMainWindowRecursive(DWORD processId, const QJsonObject& windowHints, int depth = 0, int maxDepth = 4);
};

#endif // SYSTEMINTERACTIONMODULE_H 