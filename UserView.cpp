#include "UserView.h"
#include "AppCardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>
#include <QDebug>
#include <QPainter>
#include <QTimer>
#include <QResizeEvent>
#include <QPixmap>
#include "AppStatusBar.h"
#include "AppStatusModel.h"
#include "SystemInteractionModule.h"
#include <QFile>

//======================== UserView类实现 ========================

// 构造函数：初始化成员变量，设置UI和默认背景
UserView::UserView(QWidget *parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_dockFrame(nullptr),
      m_dockScrollArea(nullptr),
      m_dockScrollContentWidget(nullptr),
      m_dockItemsLayout(nullptr),
      m_isFirstShow(true)
{
    qDebug() << "用户视图(UserView): 已创建。";
    setupUi(); // 初始化界面
    setCurrentBackground(":/images/user_bg.jpg"); // 设置默认背景图
    this->setObjectName("userView"); 
}

// 析构函数：清理资源
UserView::~UserView()
{
    qDebug() << "用户视图(UserView): 已销毁。";
    clearAppCards(); // 清理所有App卡片和定时器
}

// 初始化界面布局和控件
void UserView::setupUi() {
    // 主垂直布局
    m_mainLayout = new QVBoxLayout(this);//创建一个垂直布局
    m_mainLayout->setContentsMargins(0, 0, 0, 0);//设置边距
    m_mainLayout->setSpacing(0);//设置间距

    // 水平居中布局，用于Dock栏整体居中
    QHBoxLayout* centeringLayout = new QHBoxLayout();//创建一个水平布局
    centeringLayout->setContentsMargins(0,0,0,0); //设置边距
    centeringLayout->setSpacing(0);//设置间距

    // Dock栏背景Frame，设置透明和固定高度
    m_dockFrame = new QFrame(this);//创建一个QFrame对象
    m_dockFrame->setObjectName("dockFrame"); // 设置唯一objectName，便于QSS美化
    m_dockFrame->setFixedHeight(256);//设置固定高度
    m_dockFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);//设置大小策略

    // Dock栏内部垂直布局
    QVBoxLayout* dockFrameInternalLayout = new QVBoxLayout(m_dockFrame); 
    dockFrameInternalLayout->setContentsMargins(0, 5, 0, 5);
    
    // Dock栏滚动区域，横向滚动，禁止竖直滚动
    m_dockScrollArea = new QScrollArea(m_dockFrame); 
    m_dockScrollArea->setObjectName("dockScrollArea");
    m_dockScrollArea->setWidgetResizable(true);
    m_dockScrollArea->setFrameShape(QFrame::NoFrame);
    m_dockScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dockScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_dockScrollArea->setFixedHeight(256); 
    m_dockScrollArea->setAlignment(Qt::AlignCenter);

    // Dock栏内容区，承载所有App卡片
    m_dockScrollContentWidget = new QWidget(); 
    m_dockScrollContentWidget->setObjectName("dockScrollContentWidget");
    m_dockScrollArea->setWidget(m_dockScrollContentWidget);

    // Dock栏卡片横向布局，卡片居中排列
    m_dockItemsLayout = new QHBoxLayout(m_dockScrollContentWidget);
    m_dockItemsLayout->setContentsMargins(5, 0, 5, 0);
    m_dockItemsLayout->setSpacing(5);
    m_dockItemsLayout->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    dockFrameInternalLayout->addWidget(m_dockScrollArea);
    m_dockFrame->setLayout(dockFrameInternalLayout);

    // Dock栏整体居中
    centeringLayout->addStretch(0);
    centeringLayout->addWidget(m_dockFrame, 1);
    centeringLayout->addStretch(0);
    m_mainLayout->addSpacerItem(new QSpacerItem(0, 120, QSizePolicy::Minimum, QSizePolicy::Fixed)); // 顶部120像素
    m_mainLayout->addLayout(centeringLayout);
    m_mainLayout->addStretch(1); // 下方弹性空间

    // 应用状态栏控件和数据模型
    m_statusModel = new AppStatusModel(this);//创建一个AppStatusModel对象
    m_statusBar = new AppStatusBar(this);//创建一个AppStatusBar对象
    m_statusBar->setModel(m_statusModel);//设置模型
    m_mainLayout->addWidget(m_statusBar); // 添加到底部

    // 定时刷新应用状态，每秒刷新一次
    m_statusRefreshTimer = new QTimer(this);
    m_statusRefreshTimer->setInterval(1000);
    connect(m_statusRefreshTimer, &QTimer::timeout, this, [this]() {
        // 定时获取所有应用状态并刷新状态栏
        extern SystemInteractionModule* systemInteractionModule; // 需在主程序中定义
        if (systemInteractionModule) {
            QList<AppStatus> statusList = systemInteractionModule->getAllAppStatus(m_currentApps);
            m_statusModel->updateStatus(statusList);
        }
    });
    m_statusRefreshTimer->start();

    setLayout(m_mainLayout);
    this->setObjectName("userView");
    // 加载并应用 UserView.qss 样式表
    QFile userViewStyleFile(":/styles/UserView.qss");
    if (userViewStyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString userViewStyleSheet = QLatin1String(userViewStyleFile.readAll());
        this->setStyleSheet(userViewStyleSheet);
        userViewStyleFile.close();
    } else {
        qWarning() << "UserView: Could not load QSS file.";
    }
}

// 清空所有App卡片及相关定时器，释放资源
void UserView::clearAppCards() {
    for (AppCardWidget* card : qAsConst(m_appCards)) {
        if (card) {
            m_dockItemsLayout->removeWidget(card);
            card->deleteLater();
        }
    }
    m_appCards.clear();
    qDeleteAll(m_launchTimers);
    m_launchTimers.clear();
    m_launchingApps.clear();
}

// 设置当前应用列表并刷新界面
void UserView::setAppList(const QList<AppInfo>& apps) {
    m_currentApps = apps;
    qDebug() << "UserView::setAppList - 收到新白名单，数量:" << apps.count() << ", 当前视图可见:" << isVisible();
    populateAppList(m_currentApps);
}

// 首次显示时填充应用列表
void UserView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_isFirstShow) {
        populateAppList(m_currentApps);
        m_isFirstShow = false;
    }
}

// 响应窗口尺寸变化，调整Dock栏宽度
void UserView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateDockFrameOptimalWidth();
    updateDockItemsLayoutAlignment();
}

void UserView::updateDockItemsLayoutAlignment()
{
    if (!m_dockItemsLayout || !m_dockScrollArea || !m_dockScrollContentWidget) return;

    // 计算所有卡片总宽度
    int totalCardWidth = 0;
    for (AppCardWidget* card : m_appCards) {
        totalCardWidth += card->width() + m_dockItemsLayout->spacing();
    }
    int visibleWidth = m_dockScrollArea->viewport()->width();

    // 设置内容区最小宽度，保证未溢出时居中
    if (totalCardWidth < visibleWidth) {
        m_dockScrollContentWidget->setMinimumWidth(visibleWidth);
    } else {
        m_dockScrollContentWidget->setMinimumWidth(totalCardWidth);
    }
}

// 填充应用卡片到Dock栏，先清空再插入新卡片
void UserView::populateAppList(const QList<AppInfo>& apps) {
    qDebug() << "UserView::populateAppList - Received" << apps.count() << "apps. Clearing existing cards.";
    for (const AppInfo& app : apps) {
        qDebug() << "  App:" << app.name << "Path:" << app.path << "Icon isNull:" << app.icon.isNull();
    }
    clearAppCards();
    m_currentApps = apps;
    
    if (!m_dockItemsLayout) {
        qWarning() << "UserView::populateAppList: m_dockItemsLayout is null!";
        return;
    }

    // 居中插入卡片，实现从中间向两边扩散排列
    for (const AppInfo& appInfo : qAsConst(m_currentApps)) {
        QIcon icon = appInfo.icon;
        if (icon.isNull()) {
            // 只在首次发现icon为null时警告，避免重复
            static QSet<QString> warnedApps;
            if (!warnedApps.contains(appInfo.name)) {
                qWarning() << "UserView: Icon for" << appInfo.name << "is null. AppCardWidget will use default icon.";
                warnedApps.insert(appInfo.name);
            }
        }
        AppCardWidget *card = new AppCardWidget(appInfo.name, appInfo.path, icon, m_dockScrollContentWidget);
        connect(card, &AppCardWidget::launchAppRequested, this, &UserView::onCardLaunchRequested);
        int mid = m_dockItemsLayout->count() / 2;
        m_dockItemsLayout->insertWidget(mid, card);
        m_appCards.append(card);
    }
    if (m_dockScrollContentWidget) { 
        m_dockScrollContentWidget->adjustSize();
    }
    updateDockFrameOptimalWidth();
}

// 动态调整Dock栏宽度（如需自适应，当前由布局和QSS控制）
void UserView::updateDockFrameOptimalWidth() {
    if (!m_dockFrame || !m_dockScrollContentWidget || !this->parentWidget()) {
        return;
    }
    // 目前宽度由布局和QSS控制，无需手动设置
}

// 设置背景图片，imagePath为空则清除背景
void UserView::setCurrentBackground(const QString& imagePath) {
    if (imagePath.isEmpty()) {
        m_currentBackground = QPixmap();
        qDebug() << "UserView: Background image cleared.";
    } else {
        if (!m_currentBackground.load(imagePath)) {
            qWarning() << "UserView: Failed to load background image from" << imagePath;
            m_currentBackground = QPixmap();
        }
         qDebug() << "UserView: Background image set to" << imagePath;
    }
    update(); // 触发重绘
}

// 绘制背景图片或默认背景色
void UserView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    if (!m_currentBackground.isNull()) {
        painter.drawPixmap(this->rect(), m_currentBackground);
    } else {
        painter.fillRect(this->rect(), QColor(30, 30, 30)); // 默认深色背景
    }
}

// 根据应用路径查找对应的App卡片，找不到返回nullptr
AppCardWidget* UserView::findAppCardByPath(const QString& appPath) const {
    for (AppCardWidget* card : qAsConst(m_appCards)) {
        if (card && card->getAppPath() == appPath) {
            return card;
        }
    }
    return nullptr;
}

// 处理应用卡片的启动请求信号，转发为UserView信号
void UserView::onCardLaunchRequested(const QString& appPath, const QString& appName) {
    qDebug() << "UserView: Launch requested for" << appName << "at" << appPath;
    emit applicationLaunchRequested(appPath, appName);
}

// 设置指定应用的加载中状态，并管理超时定时器
void UserView::setAppLoadingState(const QString& appPath, bool isLoading) {
    AppCardWidget* card = findAppCardByPath(appPath);
    if (card) {
        card->setLoadingState(isLoading);
    } else {
        qWarning() << "UserView: Could not find app card for path:" << appPath << "to set loading state.";
    }
    if (isLoading) {
        // 启动时添加定时器，超时自动重置状态
        if (!m_launchingApps.contains(appPath)) {
            m_launchingApps.insert(appPath);
            QTimer* timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this, [this, appPath]() { this->onLaunchTimerTimeout(appPath); });
            m_launchTimers.insert(appPath, timer);
            timer->start(LAUNCH_TIMEOUT_MS);
        }
    } else {
        // 结束时移除定时器
        m_launchingApps.remove(appPath);
        if (m_launchTimers.contains(appPath)) {
            QTimer* timer = m_launchTimers.value(appPath);
            if (timer) {
                timer->stop();
                timer->deleteLater();
            }
            m_launchTimers.remove(appPath);
        }
    }
}

// 启动超时处理槽函数，超时后重置加载状态
void UserView::onLaunchTimerTimeout(const QString& appPath) {
    if (m_launchingApps.contains(appPath)) {
        qWarning() << "UserView: Launch timed out for" << appPath;
        this->setAppLoadingState(appPath, false);
    } else {
        qWarning() << "UserView: Launch timer for" << appPath << "fired, but app not in launching state or path mismatch.";
    }
}

// 设置状态栏高亮指定应用
void UserView::setActiveAppInStatusBar(const QString& appPath)
{
    if (m_statusModel) {
        m_statusModel->setActiveApp(appPath);
    }
}
