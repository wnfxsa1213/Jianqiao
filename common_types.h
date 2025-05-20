#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString> // For QString
#include <QIcon>   // Added for WhitelistedApp::icon

// Represents an application in the whitelist
struct AppInfo {
    QString name;
    QString path;
    QIcon icon; // Added to store the application's icon

    // Optional: Add an equality operator if not already present and needed for QList comparisons etc.
    bool operator==(const AppInfo& other) const {
        return name == other.name && path == other.path;
    }
};

// Add other common structs or enums here in the future if needed.

#endif // COMMON_TYPES_H 