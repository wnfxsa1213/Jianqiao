#ifndef USERVIEW_H
#define USERVIEW_H

#include "common_types.h"
#include <QWidget>
#include <QList>
#include <QSet>
#include <QHash>
#include <QTimer>
#include <QVBoxLayout>   // For main layout
#include <QHBoxLayout>   // For centering dock
#include <QFrame>        // For the dock background
#include "HoverIconWidget.h" 
#include <QPixmap> // ADDED for m_currentBackground

// Forward declarations
// class QLabel; // No longer used
class QScrollArea;
class QGridLayout;
// struct WhitelistedApp; // No longer needed, full definition in common_types.h

const int LAUNCH_TIMEOUT_MS = 30000; // 30 seconds

class UserView : public QWidget
{
    Q_OBJECT

public:
    explicit UserView(QWidget *parent = nullptr);
    ~UserView();

    void populateAppList(const QList<AppInfo>& apps);
    void setAppList(const QList<AppInfo>& apps);
    void setCurrentBackground(const QString& imagePath); // ADDED

    // Methods to manage icon state during app launch
    void setAppIconLaunching(const QString& appPath, bool isLaunching); // ADDED
    void resetAppIconState(const QString& appPath);                   // ADDED

signals:
    void applicationLaunchRequested(const QString& appPath);
    // void requestExitUserMode(); // Placeholder for future use, keep if needed

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override; // ADDED override

private slots:
    // void onAppItemDoubleClicked(QListWidgetItem *item); // Removed
    void onIconClicked(const QString& appPath);
    // void onExitApplicationClicked(); // REMOVED slot
    // void onCardLaunchRequested(const QString& appPath, const QString& appName); // Removed
    void onLaunchTimerTimeout(const QString& appPath); // ADDED slot

private:
    void setupUi();
    void clearAppIcons(); // Helper to clear icons from dockLayout
    HoverIconWidget* findAppCardByPath(const QString& appPath) const; // ADDED helper

    QVBoxLayout* m_mainLayout;       // Main layout for UserView
    QFrame* m_dockFrame;             // The dock panel
    QHBoxLayout* m_dockLayout;       // Layout for icons inside the dock
    QList<HoverIconWidget*> m_appIconWidgets; // Keep track of icon widgets
    // QPushButton* m_exitApplicationButton; // REMOVED exit button

    bool m_isFirstShow = true;         // Flag for first show event
    QList<AppInfo> m_cachedAppList; // Cache for app list

    QPixmap m_currentBackground; // ADDED

    QSet<QString> m_launchingApps;      // ADDED: Tracks apps currently being launched
    QHash<QString, QTimer*> m_launchTimers; // ADDED: Timers for launch timeout

    // Removed members related to QScrollArea and QGridLayout
    // QScrollArea* m_scrollArea;
    // QWidget* m_scrollAreaWidgetContents;
    // QGridLayout* m_gridLayout;
    // QList<AppCardWidget*> m_appCards;
};

#endif // USERVIEW_H 