#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>     // For QString
#include <QIcon>       // For QIcon in AppInfo
#include <QJsonObject> // For QJsonObject in AppInfo and SuggestedWindowHints
#include <QMetaType>   // For Q_DECLARE_METATYPE
#include <windows.h>   // For HWND, DWORD (used in SuggestedWindowHints)

// Represents an application in the whitelist
struct AppInfo {
    QString name;
    QString path;
    QIcon icon;
    QString mainExecutableHint;
    QJsonObject windowFindingHints; // Renamed from windowHints to windowFindingHints for clarity

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

    // This might be redundant if detectedExecutableName IS the main one after resolving launcher
    // Or, if detectedExecutableName is the launcher, and this is the target.
    // Let's assume detectedMainExecutableName refers to the actual main app (e.g. wps.exe),
    // while detectedExecutableName could be the initial process (e.g. ksolaunch.exe OR wps.exe).
    QString detectedMainExecutableName;

    SuggestedWindowHints() = default; // Ensure default constructor is available

    QString toString() const {
        return QString("SuggestedHints: Valid:%1, Exe:'%2'(PID:%3), MainTargetExe:'%4', HWND:%5, Class:'%6', Title:'%7', TopLevel:%8, AppWindowStyle:%9")
            .arg(isValid)
            .arg(detectedExecutableName)
            .arg(processId)
            .arg(detectedMainExecutableName)
            .arg(reinterpret_cast<quintptr>(windowHandle))
            .arg(detectedClassName)
            .arg(exampleTitle)
            .arg(isTopLevel)
            .arg(hasAppWindowStyle);
    }
};
Q_DECLARE_METATYPE(SuggestedWindowHints)

// Make AppInfo also known to the meta-object system if it might be used in queued connections or QVariant
// Q_DECLARE_METATYPE(AppInfo) // AppInfo contains QIcon which is not trivially copiable for metatype by default without further registration if icon is complex.
                           // For now, let's assume AppInfo is not directly passed in queued signals that would require deep copy via metatype.

#endif // COMMON_TYPES_H 