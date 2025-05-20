#ifndef USERVIEW_H
#define USERVIEW_H

#include "common_types.h"
#include <QWidget>
#include <QList>
#include <QVBoxLayout>   // For main layout
#include <QHBoxLayout>   // For centering dock
#include <QFrame>        // For the dock background
#include "HoverIconWidget.h" 

// Forward declarations
// class QLabel; // No longer used
class QScrollArea;
class QGridLayout;
// struct WhitelistedApp; // No longer needed, full definition in common_types.h

class UserView : public QWidget
{
    Q_OBJECT

public:
    explicit UserView(QWidget *parent = nullptr);
    ~UserView();

    void populateAppList(const QList<AppInfo>& apps);
    void setAppList(const QList<AppInfo>& apps);

signals:
    void applicationLaunchRequested(const QString& appPath);
    // void requestExitUserMode(); // Placeholder for future use, keep if needed

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    // void onAppItemDoubleClicked(QListWidgetItem *item); // Removed
    void onIconClicked(const QString& appPath);
    // void onExitApplicationClicked(); // REMOVED slot
    // void onCardLaunchRequested(const QString& appPath, const QString& appName); // Removed

private:
    void setupUi();
    void clearAppIcons(); // Helper to clear icons from dockLayout

    QVBoxLayout* m_mainLayout;       // Main layout for UserView
    QFrame* m_dockFrame;             // The dock panel
    QHBoxLayout* m_dockLayout;       // Layout for icons inside the dock
    QList<HoverIconWidget*> m_appIconWidgets; // Keep track of icon widgets
    // QPushButton* m_exitApplicationButton; // REMOVED exit button

    bool m_isFirstShow = true;         // Flag for first show event
    QList<AppInfo> m_cachedAppList; // Cache for app list

    // Removed members related to QScrollArea and QGridLayout
    // QScrollArea* m_scrollArea;
    // QWidget* m_scrollAreaWidgetContents;
    // QGridLayout* m_gridLayout;
    // QList<AppCardWidget*> m_appCards;
};

#endif // USERVIEW_H 