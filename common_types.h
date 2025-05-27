#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString> // For QString
#include <QIcon>   // Added for WhitelistedApp::icon
#include <QJsonObject> // Added for QJsonObject

// Represents an application in the whitelist
struct AppInfo {
    QString name;
    QString path;
    QIcon icon; // Added to store the application's icon
    QString mainExecutableHint; // Added to store the main executable name for launcher apps
    QJsonObject windowFindingHints; // New: For detailed window matching

    // Optional: Add an equality operator if not already present and needed for QList comparisons etc.
    bool operator==(const AppInfo& other) const {
        return name == other.name && path == other.path;
    }
};

// Add other common structs or enums here in the future if needed.

#endif // COMMON_TYPES_H 