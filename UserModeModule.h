#ifndef USERMODEMODULE_H
#define USERMODEMODULE_H

#include "common_types.h"
#include <QObject>
#include <QList>
#include <QMap> // For QMap
#include <QProcess>
#include <QTimer>
#include "UserView.h"       // For m_userView interaction
#include "SystemInteractionModule.h" // For icon fetching and process interaction
#include "JianqiaoCoreShell.h"
#include <QSet>

// 前置声明
class UserView;
class SystemInteractionModule;

/**
 * @brief 用户模式主控模块，负责用户模式下的应用白名单管理、进程启动、界面交互等。
 * 负责与UserView、SystemInteractionModule等模块协作。
 */
class UserModeModule : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param coreShell 主程序核心指针
     * @param userView 用户视图指针
     * @param systemInteraction 系统交互模块指针
     * @param parent 父对象
     */
    explicit UserModeModule(JianqiaoCoreShell *coreShell, UserView* userView, SystemInteractionModule* systemInteraction, QObject *parent = nullptr);
    /**
     * @brief 析构函数，释放资源
     */
    ~UserModeModule();

    /**
     * @brief 激活用户模式，显示UserView并刷新白名单
     */
    void activate();
    /**
     * @brief 关闭用户模式，隐藏UserView
     */
    void deactivate();
    /**
     * @brief 加载白名单配置并刷新UserView
     */
    void loadAndSetWhitelist();

    /**
     * @brief 显示用户视图
     */
    void showUserView();
    /**
     * @brief 隐藏用户视图
     */
    void hideUserView();
    /**
     * @brief 判断用户视图是否可见
     * @return 是否可见
     */
    bool isUserViewVisible() const;
    /**
     * @brief 获取用户视图QWidget指针
     */
    QWidget* getUserViewWidget();
    /**
     * @brief 获取UserView实例指针
     */
    UserView* getViewInstance();

    /**
     * @brief 设置UserView实例指针
     * @param view UserView指针
     */
    void setUserViewInstance(UserView* view);

    /**
     * @brief 加载配置文件，填充白名单应用列表
     */
    void loadConfiguration();
    /**
     * @brief 判断某进程名/路径是否在白名单
     * @param processName 可执行文件名或路径
     * @return 是否在白名单
     */
    bool isAppWhitelisted(const QString& processName) const;
    /**
     * @brief 根据应用名获取路径
     * @param appName 应用名
     * @return 路径
     */
    QString getAppPathForName(const QString& appName) const;
    /**
     * @brief 更新白名单应用列表
     * @param apps 新的应用列表
     */
    void updateUserAppList(const QList<AppInfo>& apps);
    /**
     * @brief 终止所有已启动的进程
     */
    void terminateActiveProcesses();

    /**
     * @brief 获取配置文件路径（静态方法）
     * @return 配置文件绝对路径
     */
    static QString getConfigFilePath();

signals:
    /** 用户模式激活信号 */
    void userModeActivated();
    /** 用户模式关闭信号 */
    void userModeDeactivated();
    /** 应用启动失败信号 */
    void applicationFailedToLaunch(const QString& appName, const QString& error);
    /** 白名单应用列表更新信号 */
    void userAppListUpdated(const QList<AppInfo>& apps);

public slots:
    /**
     * @brief 处理应用启动请求
     * @param appPath 应用路径
     * @param appName 应用名称
     */
    void onApplicationLaunchRequested(const QString& appPath, const QString& appName);
    /**
     * @brief 进程状态变化槽
     * @param newState 新状态
     */
    void onProcessStateChanged(QProcess::ProcessState newState);
    /**
     * @brief 应用窗口激活成功槽
     * @param appPath 应用路径
     */
    void onApplicationActivated(const QString& appPath);
    /**
     * @brief 应用窗口激活失败槽
     * @param appPath 应用路径
     */
    void onApplicationActivationFailed(const QString& appPath);
    /**
     * @brief 进程启动完成槽
     * @param appPath 应用路径
     */
    void onProcessStarted(const QString& appPath);
    /**
     * @brief 进程结束槽
     * @param appPath 应用路径
     * @param exitCode 退出码
     * @param exitStatus 退出状态
     */
    void onProcessFinished(const QString& appPath, int exitCode, QProcess::ExitStatus exitStatus);
    /**
     * @brief 进程错误槽
     * @param appPath 应用路径
     * @param error 错误类型
     */
    void onProcessError(const QString& appPath, QProcess::ProcessError error);

private:
    /**
     * @brief 启动应用进程
     * @param appPath 应用路径
     * @param appName 应用名称
     */
    void launchApplication(const QString& appPath, const QString& appName);
    /**
     * @brief 监控已启动进程
     * @param process 进程指针
     * @param appName 应用名称
     */
    void monitorLaunchedProcess(QProcess* process, const QString& appName);
    /**
     * @brief 查找可执行文件名
     * @param appPath 应用路径
     * @return 可执行文件名
     */
    QString findExecutableName(const QString& appPath) const;
    /**
     * @brief 启动进程监控定时器
     */
    void startProcessMonitoringTimer();
    /**
     * @brief 定时监控所有已启动进程
     */
    void monitorLaunchedProcesses();
    /**
     * @brief 根据进程指针查找应用路径
     * @param process 进程指针
     * @return 应用路径
     */
    QString findAppPathForProcess(QProcess* process);

    JianqiaoCoreShell *m_coreShellPtr; ///< 主程序核心指针
    UserView *m_userViewPtr; ///< 用户视图指针
    SystemInteractionModule *m_systemInteractionModulePtr; ///< 系统交互模块指针
    QList<AppInfo> m_whitelistedApps; ///< 白名单应用列表
    QHash<QString, QProcess*> m_launchedProcesses; ///< 已启动进程映射（appPath->QProcess*）
    QTimer* m_processMonitoringTimer; ///< 进程监控定时器
    bool m_configLoaded = false; ///< 配置是否已加载
    QMap<QString, QTimer*> m_launchTimers; ///< 启动超时定时器映射
    QSet<QString> m_launchingApps; ///< 正在启动的应用路径集合
    QSet<QString> m_pendingActivationApps; ///< 等待激活的应用路径集合
    QString m_configFilePath; ///< 配置文件路径
};

#endif // USERMODEMODULE_H 