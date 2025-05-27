#include "JianqiaoCoreShell.h"
#include "SystemInteractionModule.h"
#include "AdminModule.h"
#include "UserModeModule.h"
#include "UserView.h"
#include "AdminDashboardView.h"
#include <QStackedWidget>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QWindow>
#include <windows.h>

JianqiaoCoreShell::JianqiaoCoreShell(QWidget *parent)
    : QMainWindow(parent)
    , m_systemInteractionModule(nullptr)
    , m_adminModule(nullptr)
    , m_userModeModule(nullptr)
    , m_userViewInstance(nullptr)
    , m_adminDashboardInstance(nullptr)
    , m_mainStackedWidget(nullptr)
    , m_currentMode(OperatingMode::UserMode)
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 构造函数开始。";

    initializeCoreApplication();
    setupUi();
    initializeConnections();

    switchToUserModeView();
    if (m_userModeModule) {
        qDebug() << "剑鞘核心(JianqiaoCoreShell): 用户视图已显示 (初始状态)。";
    }

    qDebug() << "剑鞘核心(JianqiaoCoreShell): 构造函数结束。";
}

void JianqiaoCoreShell::initializeCoreApplication()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 初始化核心应用设置。";
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    this->setAttribute(Qt::WA_TranslucentBackground, false);
    this->setStyleSheet("QMainWindow { background-color: #222222; }");

    m_systemInteractionModule = new SystemInteractionModule(this);
    
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 底层系统交互模块已实例化。";
}

void JianqiaoCoreShell::setupUi()
{
    QRect primaryScreenGeometry = QGuiApplication::primaryScreen()->geometry();
    setGeometry(primaryScreenGeometry);

    m_mainStackedWidget = new QStackedWidget(this);

    m_userViewInstance = new UserView(this);
    if (!m_systemInteractionModule) {
        qCritical() << "JianqiaoCoreShell::setupUi - m_systemInteractionModule is null before creating AdminDashboardView!";
    }
    m_adminDashboardInstance = new AdminDashboardView(m_systemInteractionModule, this);

    m_mainStackedWidget->addWidget(m_userViewInstance);
    m_mainStackedWidget->addWidget(m_adminDashboardInstance);

    setCentralWidget(m_mainStackedWidget);

    qDebug() << "剑鞘核心外壳: UI设置完成 (QStackedWidget), 视图实例已创建。";
}

void JianqiaoCoreShell::initializeConnections()
{
    m_adminModule = new AdminModule(m_systemInteractionModule, m_adminDashboardInstance, this);
    m_userModeModule = new UserModeModule(this, m_userViewInstance, m_systemInteractionModule, this);

    if (m_systemInteractionModule) {
        m_systemInteractionModule->installKeyboardHook();
        connect(m_systemInteractionModule, &SystemInteractionModule::adminLoginRequested,
                this, &JianqiaoCoreShell::handleAdminLoginRequested);
        qDebug() << "剑鞘核心(JianqiaoCoreShell): 已连接 SystemInteractionModule::adminLoginRequested 信号。";
    }

    if (m_adminModule) {
        connect(m_adminModule, &AdminModule::exitAdminModeRequested,
                this, &JianqiaoCoreShell::handleExitAdminModeTriggered);
        connect(m_adminModule, &AdminModule::loginSuccessfulAndAdminActive,
                this, &JianqiaoCoreShell::handleAdminLoginSuccessful);
        qDebug() << "剑鞘核心(JianqiaoCoreShell): 已连接 AdminModule 信号 (exitAdminModeRequested, loginSuccessfulAndAdminActive)。";
    }
}

void JianqiaoCoreShell::switchToUserModeView()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 切换到用户模式视图.";

    if (m_adminModule && m_userModeModule) {
        QList<AppInfo> currentAdminApps = m_adminModule->getWhitelistedApps();
        qDebug() << "JianqiaoCoreShell: Fetched" << currentAdminApps.count() << "apps from AdminModule's current state for UserModeModule.";
        m_userModeModule->updateUserAppList(currentAdminApps);
    } else {
        qWarning() << "JianqiaoCoreShell: AdminModule or UserModeModule is null. UserModeModule may load from its own config.";
        if (m_userModeModule) {
            m_userModeModule->loadConfiguration(); // Fallback to loading from file if direct sync is not possible
        }
    }

    m_systemInteractionModule->setUserModeActive(true);
    if (m_userViewInstance) { // Ensure m_userViewInstance is not null
        m_mainStackedWidget->setCurrentWidget(m_userViewInstance);
        qDebug() << "剑鞘核心(JianqiaoCoreShell): UserView is now current widget.";
        this->activateWindow();
        this->raise();
        m_userViewInstance->setFocus(Qt::OtherFocusReason);
    } else {
        qCritical() << "剑鞘核心(JianqiaoCoreShell): m_userViewInstance is null, cannot switch widget!";
    }
    m_currentMode = OperatingMode::UserMode;
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 已切换到用户模式.";
    emit userModeActivated(); // Ensure this signal is emitted if other parts of the system need to react.
}

void JianqiaoCoreShell::switchToAdminDashboard()
{
    qDebug() << "JianqiaoCoreShell::switchToAdminDashboard() EXECUTED.";
    if (!m_adminDashboardInstance) {
        qCritical() << "AdminDashboardView instance is null in switchToAdminDashboard!";
        return;
    }
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 切换到管理员仪表盘。";
    if (m_adminModule && m_adminDashboardInstance) {
        m_adminDashboardInstance->setWhitelistedApps(m_adminModule->getWhitelistedApps());
        if (m_systemInteractionModule) {
            m_adminDashboardInstance->setCurrentAdminLoginHotkey(m_systemInteractionModule->getCurrentAdminLoginHotkeyStrings());
        }
        qDebug() << "剑鞘核心(JianqiaoCoreShell): AdminDashboardView data populated.";
    }
    if (m_mainStackedWidget && m_adminDashboardInstance) {
        m_mainStackedWidget->setCurrentWidget(m_adminDashboardInstance);
        qDebug() << "剑鞘核心(JianqiaoCoreShell): AdminDashboard is now current widget. StackedWidget Geometry:" << m_mainStackedWidget->geometry();
        this->activateWindow();
        this->raise();
        m_adminDashboardInstance->setFocus(Qt::OtherFocusReason);
    } else {
        qWarning() << "剑鞘核心(JianqiaoCoreShell): StackedWidget 或 AdminDashboard 为空，无法切换!";
    }
}

void JianqiaoCoreShell::handleAdminLoginRequested()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 接收到管理员登录请求。";
    m_currentMode = OperatingMode::AdminModePendingLogin;
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 系统模式设置为 AdminModePendingLogin.";

    if (m_systemInteractionModule) {
        m_systemInteractionModule->setUserModeActive(false);
    }
    if (m_adminModule) {
        m_adminModule->showLoginView();
        qDebug() << "剑鞘核心(JianqiaoCoreShell): 管理员登录视图已请求显示。";
    }
}

void JianqiaoCoreShell::handleExitAdminModeTriggered()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 接收到退出管理员模式请求。";
    m_currentMode = OperatingMode::UserMode;
    if (m_systemInteractionModule) {
        m_systemInteractionModule->setUserModeActive(true);
    }
    
    switchToUserModeView();

    qDebug() << "剑鞘核心(JianqiaoCoreShell): 已切换到用户模式。";
}

void JianqiaoCoreShell::handleAdminLoginSuccessful()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 管理员登录成功并处于活动状态。";
    m_currentMode = OperatingMode::AdminModeActive;
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 系统模式设置为 AdminModeActive.";

    if (m_adminModule) {
        m_adminModule->prepareAdminDashboardData();
    }

    switchToAdminDashboard();
}

void JianqiaoCoreShell::onAdminViewVisibilityChanged(bool visible)
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): onAdminViewVisibilityChanged(" << visible << ")";

    HWND currentHwnd = reinterpret_cast<HWND>(winId());
    if (!currentHwnd) {
        qWarning() << "剑鞘核心(JianqiaoCoreShell): onAdminViewVisibilityChanged - 无法获取主窗口句柄。";
        return;
    }

    if (visible) {
        if (m_adminModule && m_adminModule->isLoginViewActive()) { 
             qDebug() << "剑鞘核心(JianqiaoCoreShell): 管理员登录视图激活，主窗口设置为 HWND_NOTOPMOST。";
            ::SetWindowPos(currentHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        } else {
            qDebug() << "剑鞘核心(JianqiaoCoreShell): onAdminViewVisibilityChanged(true) called, but not for login view, or AdminModule unavailable. No change to Z-order.";
        }
    } else { // visible == false, meaning admin interaction is ending
        qDebug() << "剑鞘核心(JianqiaoCoreShell): Admin interaction ending. Ensuring shell is TOPMOST.";
        ::SetWindowPos(currentHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        qDebug() << "剑鞘核心(JianqiaoCoreShell): 主窗口恢复为 HWND_TOPMOST。";
        
        if (m_currentMode == OperatingMode::AdminModePendingLogin && m_adminModule && !m_adminModule->isAnyViewVisible()) {
             qDebug() << "剑鞘核心(JianqiaoCoreShell): 管理员登录视图关闭，未登录。切换回用户模式。";
             handleExitAdminModeTriggered();
        }
    }
}

JianqiaoCoreShell::~JianqiaoCoreShell()
{
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 已销毁。";
}

bool JianqiaoCoreShell::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        HWND shellHwnd = reinterpret_cast<HWND>(winId());
        if (shellHwnd && ::GetForegroundWindow() != shellHwnd) {
            if (QApplication::activeModalWidget()) {
                qDebug() << "剑鞘核心外壳: 窗口激活事件, 但有模态对话框 (" << QApplication::activeModalWidget()->objectName() << ") 活动。不强制前景。";
            } else if (m_mainStackedWidget && 
                       (m_mainStackedWidget->currentWidget() == m_userViewInstance || m_mainStackedWidget->currentWidget() == m_adminDashboardInstance)) {
                qDebug() << "剑鞘核心外壳: 窗口激活事件 (WindowActivate). 尝试将主外壳设为前景。";
                // ::SetForegroundWindow(shellHwnd); // 注释掉此行以避免争抢焦点
            }
        }
    }
    return QMainWindow::event(event);
}

void JianqiaoCoreShell::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
}

void JianqiaoCoreShell::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
}

void JianqiaoCoreShell::onAdminRequestsExitAdminMode() {
    qDebug() << "剑鞘核心(JianqiaoCoreShell): 接收到退出管理员模式请求 (来自AdminModule).";
    // Ensure AdminModule signals view changes if necessary, or CoreShell manages view stack directly.
    // m_adminModule->hideViews(); // Example if AdminModule managed its own views directly

    switchToUserModeView(); // Centralized method to switch to user mode
}

void JianqiaoCoreShell::onAdminLoginSuccessful() {
    // ... existing code ...
} 