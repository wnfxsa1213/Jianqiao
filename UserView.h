#ifndef USERVIEW_H
#define USERVIEW_H

#include "common_types.h"
#include <QWidget>
#include <QList>
#include <QSet>
#include <QHash>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include "AppCardWidget.h"
#include <QPixmap>
#include <QtCore/QString>
#include "AppStatusBar.h"
#include "AppStatusModel.h"

// 前置声明
class QLabel;
class QFrame;
class QScrollArea;
class QHBoxLayout;
class QVBoxLayout;
class AppCardWidget;
struct AppInfo;

const int LAUNCH_TIMEOUT_MS = 30000; // 启动超时时间（毫秒）
const int APP_MAX_DOCK_WIDTH = 1200; // Dock栏最大宽度

// UserView类：用户主界面视图，包含应用Dock栏、状态栏等
class UserView : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，初始化界面
    explicit UserView(QWidget *parent = nullptr);
    // 析构函数，释放资源
    ~UserView();

    // 填充应用卡片到Dock栏，参数为应用信息列表
    void populateAppList(const QList<AppInfo>& apps);
    // 设置当前应用列表并刷新界面
    void setAppList(const QList<AppInfo>& apps);
    // 设置背景图片，imagePath为空则清除背景
    void setCurrentBackground(const QString& imagePath);
    // 设置指定应用的加载中状态，appPath为应用路径，isLoading为是否加载中
    void setAppLoadingState(const QString& appPath, bool isLoading);
    // 高亮底部状态栏指定应用
    void setActiveAppInStatusBar(const QString& appPath);

signals:
    // 应用启动请求信号，参数为应用路径和名称
    void applicationLaunchRequested(const QString& appPath, const QString& appName);

protected:
    // 首次显示事件，自动填充应用列表
    void showEvent(QShowEvent *event) override;
    // 绘制背景图片或默认背景色
    void paintEvent(QPaintEvent *event) override;
    // 响应窗口尺寸变化，调整Dock栏宽度
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // 处理卡片启动请求信号，转发为UserView信号
    void onCardLaunchRequested(const QString& appPath, const QString& appName);
    // 启动超时处理槽函数，超时后重置加载状态
    void onLaunchTimerTimeout(const QString& appPath);

private:
    // 初始化界面布局和控件
    void setupUi();
    // 清空所有App卡片及相关定时器，释放资源
    void clearAppCards();
    // 根据应用路径查找对应的App卡片，找不到返回nullptr
    AppCardWidget* findAppCardByPath(const QString& appPath) const;
    // 动态调整Dock栏宽度（如需自适应，当前由布局和QSS控制）
    void updateDockFrameOptimalWidth();
    void updateDockItemsLayoutAlignment(); // 新增：声明居中对齐辅助函数

    QVBoxLayout* m_mainLayout;       // 主垂直布局
    QFrame* m_dockFrame;             // Dock栏背景Frame
    QScrollArea* m_dockScrollArea;   // Dock栏滚动区域
    QWidget* m_dockScrollContentWidget; // Dock栏内容区
    QHBoxLayout* m_dockItemsLayout;  // Dock栏卡片横向布局

    QList<AppCardWidget*> m_appCards; // 当前所有App卡片
    QList<AppInfo> m_currentApps;     // 当前应用列表
    QPixmap m_currentBackground;      // 当前背景图片

    bool m_isFirstShow = true;        // 是否首次显示

    QSet<QString> m_launchingApps;    // 正在启动的应用路径集合
    QHash<QString, QTimer*> m_launchTimers; // 启动超时定时器映射

    AppStatusBar* m_statusBar = nullptr;    // 应用状态栏控件
    AppStatusModel* m_statusModel = nullptr; // 应用状态数据模型
    QTimer* m_statusRefreshTimer = nullptr;  // 状态定时刷新定时器
};

#endif // USERVIEW_H 