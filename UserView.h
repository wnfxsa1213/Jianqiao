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
#include "AppCardWidget.h" // CHANGED from HoverIconWidget
#include <QPixmap> // ADDED for m_currentBackground
#include <QtCore/QString> // More explicit include for QString
#include "AppStatusBar.h" // 新增：应用状态栏控件
#include "AppStatusModel.h" // 新增：应用状态模型

// Forward declarations
class QLabel;
class QFrame;
class QScrollArea;
class QHBoxLayout; // 恢复QHBoxLayout前置声明
class QVBoxLayout;
class AppCardWidget;
struct AppInfo; // Forward declaration

const int LAUNCH_TIMEOUT_MS = 30000; // 30 seconds
const int APP_MAX_DOCK_WIDTH = 1200; // <-- NEW CONSTANT

class UserView : public QWidget
{
    Q_OBJECT

public:
    explicit UserView(QWidget *parent = nullptr);
    ~UserView();

    void populateAppList(const QList<AppInfo>& apps);
    void setAppList(const QList<AppInfo>& apps);
    void setCurrentBackground(const QString& imagePath); 

    void setAppLoadingState(const QString& appPath, bool isLoading); // REPLACES setAppIconLaunching & resetAppIconState

    // 新增：高亮底部状态栏指定应用
    void setActiveAppInStatusBar(const QString& appPath);

signals:
    void applicationLaunchRequested(const QString& appPath, const QString& appName); // Added appName
    // void requestExitUserMode(); // Placeholder for future use, keep if needed

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override; 
    void resizeEvent(QResizeEvent *event) override; // For responsive grid layout

private slots:
    // void onAppItemDoubleClicked(QListWidgetItem *item); // Removed
    void onCardLaunchRequested(const QString& appPath, const QString& appName); // Connect to AppCardWidget's signal
    // void onExitApplicationClicked(); // REMOVED slot
    void onLaunchTimerTimeout(const QString& appPath); 

private:
    void setupUi();
    void clearAppCards(); // Renamed from clearAppIcons
    AppCardWidget* findAppCardByPath(const QString& appPath) const; // Type changed
    // void updateGridLayout(); // Removed, grid layout is gone
    void updateDockFrameOptimalWidth(); // <-- NEW METHOD

    QVBoxLayout* m_mainLayout;       
    QFrame* m_dockFrame;            // Dock panel background and container
    QScrollArea* m_dockScrollArea;       // For horizontal scrolling of dock items
    QWidget* m_dockScrollContentWidget; // Content widget for m_dockScrollArea
    QHBoxLayout* m_dockItemsLayout;      // Layout for AppCardWidgets within m_dockScrollContentWidget

    QList<AppCardWidget*> m_appCards; // CHANGED from m_appIconWidgets
    QList<AppInfo> m_currentApps;
    QPixmap m_currentBackground; 

    bool m_isFirstShow = true;         

    QSet<QString> m_launchingApps;      
    QHash<QString, QTimer*> m_launchTimers; 

    AppStatusBar* m_statusBar = nullptr; // 应用状态栏控件
    AppStatusModel* m_statusModel = nullptr; // 应用状态数据模型
    QTimer* m_statusRefreshTimer = nullptr; // 状态定时刷新定时器
};

#endif // USERVIEW_H 