#ifndef JIANQIAOCORESHELL_H
#define JIANQIAOCORESHELL_H

#include <QMainWindow>
#include <QEvent>
#include <QStackedWidget>

class SystemInteractionModule; // Forward declaration
class AdminModule;             // Forward declaration
class UserModeModule;          // Forward declaration for UserModeModule
class UserView;                // Forward declaration for UserView, if UserModeModule's view is managed by CoreShell
class AdminDashboardView;       // Added Forward declaration

class JianqiaoCoreShell : public QMainWindow
{
    Q_OBJECT

public:
    enum class OperatingMode {
        UserMode,
        AdminModePendingLogin,
        AdminModeActive
    };

    explicit JianqiaoCoreShell(QWidget *parent = nullptr);
    ~JianqiaoCoreShell();

    // 新增：获取底层系统交互模块指针
    SystemInteractionModule* getSystemInteractionModule() const { return m_systemInteractionModule; }

signals:
    void userModeActivated();

public slots:
    void onAdminViewVisibilityChanged(bool visible);
    void handleAdminLoginRequested();
    void handleExitAdminModeTriggered();
    void handleAdminLoginSuccessful();

private:
    void setupUi();
    void initializeConnections();
    void initializeCoreApplication();
    void switchToUserModeView();    // Added helper
    void switchToAdminDashboard();   // Added helper
    void setCurrentSystemMode(OperatingMode mode);

    // Override event handler for activation changes
protected: // QWidget's event is protected
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    SystemInteractionModule *m_systemInteractionModule;
    AdminModule *m_adminModule;
    UserModeModule *m_userModeModule;
    
    // View instances managed by the stacked widget
    UserView *m_userViewInstance; // Renamed/repurposed from m_userView or m_userModeViewWidget
    AdminDashboardView *m_adminDashboardInstance; // Added

    QStackedWidget *m_mainStackedWidget; // Added

    OperatingMode m_currentMode;

private slots:
    void onAdminRequestsExitAdminMode();
};

#endif // JIANQIAOCORESHELL_H 