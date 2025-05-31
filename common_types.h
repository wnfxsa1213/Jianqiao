#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>     // For QString
#include <QIcon>       // For QIcon in AppInfo
#include <QJsonObject> // For QJsonObject in AppInfo and SuggestedWindowHints
#include <QMetaType>   // For Q_DECLARE_METATYPE
#include <QList>       // For QList in other headers
#include <QVariant>    // For QVariant if needed
#include <windows.h>   // For HWND, DWORD (used in SuggestedWindowHints)
#include <QJsonArray>  // For QJsonArray in SuggestedWindowHints

// Represents an application in the whitelist
struct AppInfo {
    QString name;
    QString path;
    QIcon icon;
    QString mainExecutableHint;
    QJsonObject windowFindingHints; // Renamed from windowHints to windowFindingHints for clarity
    QString exePath; // 新增：可执行文件完整路径，用于进程重启等
    bool smartTopmost = true; // 智能置顶
    bool forceTopmost = false; // 强力置顶

    bool operator==(const AppInfo& other) const {
        return name == other.name && path == other.path;
    }
};

// Structure to hold detected window parameters suggestions
struct SuggestedWindowHints {
    QString detectedExecutableName; // The name of the exe for which window was found (e.g. "wps.exe")
    DWORD processId = 0;
    HWND windowHandle = nullptr;
    QString detectedClassName;
    QString exampleTitle;
    bool isTopLevel = false;
    bool hasAppWindowStyle = false; // WS_EX_APPWINDOW (appears in taskbar)
    bool isValid = false;           // Overall validity of the detected hints
    QString errorString;            // To store error messages during detection
    int bestScoreDuringDetection = -1; // Score achieved by the detected window
    QString detectedMainExecutableName;
    QString processFullPath;        // 进程完整路径
    DWORD parentProcessId = 0;      // 父进程ID
    HWND parentWindowHandle = nullptr; // 父窗口句柄
    int windowHierarchyLevel = 0;   // 窗口层级（0为顶层）
    bool isVisible = false;         // 是否可见
    bool isMinimized = false;       // 是否最小化

    QJsonArray candidatesJson; // 新增：用于存储探测失败时的候选窗口信息，供UI弹窗展示

    SuggestedWindowHints() = default; // Ensure default constructor is available

    QString toString() const {
        return QString("SuggestedHints: Valid:%1, Exe:'%2'(PID:%3), MainTargetExe:'%4', HWND:%5, Class:'%6', Title:'%7', TopLevel:%8, AppWindowStyle:%9, FullPath:'%10', ParentPID:%11, ParentHWND:%12, Level:%13, Visible:%14, Minimized:%15")
            .arg(isValid)
            .arg(detectedExecutableName)
            .arg(processId)
            .arg(detectedMainExecutableName)
            .arg(reinterpret_cast<quintptr>(windowHandle))
            .arg(detectedClassName)
            .arg(exampleTitle)
            .arg(isTopLevel)
            .arg(hasAppWindowStyle)
            .arg(processFullPath)
            .arg(parentProcessId)
            .arg(reinterpret_cast<quintptr>(parentWindowHandle))
            .arg(windowHierarchyLevel)
            .arg(isVisible)
            .arg(isMinimized);
    }
};
Q_DECLARE_METATYPE(SuggestedWindowHints)

// Make AppInfo also known to the meta-object system if it might be used in queued connections or QVariant
// Q_DECLARE_METATYPE(AppInfo) // AppInfo contains QIcon which is not trivially copiable for metatype by default without further registration if icon is complex.
                           // For now, let's assume AppInfo is not directly passed in queued signals that would require deep copy via metatype.

#endif // COMMON_TYPES_H 