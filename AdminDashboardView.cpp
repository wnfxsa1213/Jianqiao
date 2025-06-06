#include "AdminDashboardView.h"
#include "UserModeModule.h"
#include "HotkeyEditDialog.h"
#include <QDebug>
#include <QPushButton> // For example exit button
#include <QFileDialog> // For onAddAppClicked
#include <QInputDialog> // For onAddAppClicked
#include <QMessageBox> // For onRemoveAppClicked
#include <QGroupBox> // For visually grouping sections
#include <QCoreApplication> // Added for QCoreApplication::quit()
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSizePolicy>
#include <QFrame> // Added for separator line
#include <QTabWidget>
#include "SystemInteractionModule.h" // Include the definition for SystemInteractionModule
#include <QProgressDialog> // For better user feedback during detection
#include "DetectionResultDialog.h" // Make sure this is included
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSettings> // 用于注册表操作
#include <QCheckBox>

AdminDashboardView::AdminDashboardView(SystemInteractionModule* systemInteractionModule, QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_whitelistTab(nullptr)
    , m_settingsTab(nullptr)
    , m_whitelistListWidget(nullptr)
    , m_addAppButton(nullptr)
    , m_detectAndAddAppButton(nullptr) // Initialize new button pointer
    , m_removeAppButton(nullptr)
    , m_currentHotkeyTitleLabel(nullptr)
    , m_currentHotkeyDisplayLabel(nullptr)
    , m_editHotkeyButton(nullptr)
    , m_currentPasswordLabel(nullptr)
    , m_currentPasswordLineEdit(nullptr)
    , m_newPasswordLabel(nullptr)
    , m_newPasswordLineEdit(nullptr)
    , m_confirmPasswordLabel(nullptr)
    , m_confirmPasswordLineEdit(nullptr)
    , m_confirmChangePasswordButton(nullptr)
    , m_exitButton(nullptr)
    , m_exitApplicationButton(nullptr)
    , m_systemInteractionModulePtr(systemInteractionModule) // Initialize the new member
    , m_detectionWaitMsSpinBox(nullptr)
    , m_saveDetectionWaitMsButton(nullptr)
    , m_autoStartCheckBox(nullptr)
    , m_detectionProgressDialog(nullptr)
    , m_smartTopmostCheckBox(nullptr)
    , m_forceTopmostCheckBox(nullptr)
{
    qDebug() << "管理员仪表盘(AdminDashboardView): 已创建。";
    setupUi();

    // Connect the detectionCompleted signal from SystemInteractionModule
    if (m_systemInteractionModulePtr) {
        connect(m_systemInteractionModulePtr, &SystemInteractionModule::detectionCompleted,
                this, &AdminDashboardView::onDetectionResultsReceived);
    }
}

AdminDashboardView::~AdminDashboardView()
{
    qDebug() << "管理员仪表盘(AdminDashboardView): 已销毁。";
}

void AdminDashboardView::setupUi()
{
    setWindowTitle("管理员仪表盘");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setObjectName("AdminDashboardView");
    this->setStyleSheet("QWidget#AdminDashboardView { background-image: url(:/images/admin_bg.jpg); background-repeat: no-repeat; background-position: center; background-attachment: fixed; }");
    // setMinimumSize(800, 600); // Increased size for more content

    // Material Design inspired stylesheet
    this->setStyleSheet(QString(
        "QWidget { background-color: #ECEFF1; color: #263238; }" // Main background and default text color
        "QLabel { background-color: transparent; }" // Ensure labels have transparent background
        "QPushButton {"
        "    background-color: #536DFE; /* Indigo A200 */"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-size: 13px;" // Slightly smaller font for material like buttons
        "    border-radius: 4px;"
        "    text-transform: uppercase;" // Material buttons often have uppercase text
        "}"
        "QPushButton:hover {"
        "    background-color: #3D5AFE; /* Indigo A400 */"
        "}"
        "QPushButton:pressed {"
        "    background-color: #304FFE; /* Indigo A700 */"
        "}"
        "QListWidget {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #CFD8DC; /* Blue Grey 100 */"
        "    border-radius: 4px;"
        "    padding: 4px;"
        "}"
        "QListWidget::item {"
        "    padding: 8px 12px; /* More padding for items */"
        "    border-bottom: 1px solid #ECEFF1; /* Separator for items */"
        "}"
        "QListWidget::item:last-child {"
        "    border-bottom: none;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #536DFE; /* Indigo A200 */"
        "    color: white;"
        "    border-radius: 2px;" // Slight radius for selected item background
        "}"
        "QLineEdit {" // Basic styling for QInputDialog's line edit if needed
        "    padding: 6px;"
        "    border: 1px solid #CFD8DC;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    min-height: 20px; /* Ensure line edits have a decent height */"
        "}"
        "QGroupBox { font-weight: bold; border: 1px solid #CFD8DC; border-radius: 4px; margin-top: 10px; }" 
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px 0 5px; background-color: #ECEFF1; }"
        "QTabWidget::pane { border: 1px solid #CFD8DC; border-top: 1px solid #CFD8DC; border-radius: 4px; background-color: #FFFFFF; margin-top: -1px; }"
        "QTabWidget::tab-bar { alignment: left; }"
        "QTabBar::tab {"
        "    background-color: #ECEFF1; /* Light background for inactive tabs */"
        "    color: #263238; /* Dark text for inactive tabs */"
        "    border: 1px solid #CFD8DC;"
        "    border-bottom: none; /* Remove bottom border for tabs */"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "    padding: 8px 16px;"
        "    margin-right: 2px; /* Space between tabs */"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #FFFFFF; /* White background for selected tab */"
        "    color: #263238; /* Dark text for selected tab */"
        "    border-color: #CFD8DC;"
        "}"
        "QTabBar::tab:hover {"
        "    background-color: #E0E0E0; /* Slightly darker on hover */"
        "}"
    ));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);

    // --- Create TabWidget and Tabs ---
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- Whitelist Tab ---
    m_whitelistTab = new QWidget(m_tabWidget);
    QVBoxLayout* whitelistTabLayout = new QVBoxLayout(m_whitelistTab);
    whitelistTabLayout->setContentsMargins(10, 10, 10, 10);
    QGroupBox* whitelistGroup = new QGroupBox("白名单应用管理", m_whitelistTab);
    QVBoxLayout* whitelistGroupLayout = new QVBoxLayout(whitelistGroup);
    m_whitelistListWidget = new QListWidget(whitelistGroup);
    m_whitelistListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_whitelistListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_whitelistListWidget->setIconSize(QSize(32, 32)); // 设置图标大小为 32x32
    whitelistGroupLayout->addWidget(m_whitelistListWidget, 1); // Give list widget stretch factor

    QHBoxLayout* whitelistButtonsLayout = new QHBoxLayout();
    m_addAppButton = new QPushButton("手动添加应用...", whitelistGroup);
    m_detectAndAddAppButton = new QPushButton("探测并添加应用...", whitelistGroup); // Create the new button
    m_removeAppButton = new QPushButton("移除选中", whitelistGroup);
    connect(m_addAppButton, &QPushButton::clicked, this, &AdminDashboardView::onAddAppClicked);
    connect(m_detectAndAddAppButton, &QPushButton::clicked, this, &AdminDashboardView::onDetectAndAddAppClicked); // Connect new button
    connect(m_removeAppButton, &QPushButton::clicked, this, &AdminDashboardView::onRemoveAppClicked);
    whitelistButtonsLayout->addStretch();
    whitelistButtonsLayout->addWidget(m_addAppButton);
    whitelistButtonsLayout->addWidget(m_detectAndAddAppButton); // Add new button to layout
    whitelistButtonsLayout->addWidget(m_removeAppButton);
    whitelistGroupLayout->addLayout(whitelistButtonsLayout);
    whitelistTabLayout->addWidget(whitelistGroup);
    m_whitelistTab->setLayout(whitelistTabLayout);
    m_tabWidget->addTab(m_whitelistTab, "白名单管理");

    // --- Settings Tab ---
    m_settingsTab = new QWidget(m_tabWidget);
    QVBoxLayout* settingsTabLayout = new QVBoxLayout(m_settingsTab);
    settingsTabLayout->setContentsMargins(10, 10, 10, 10);
    
    // --- Hotkey Settings Group ---
    QGroupBox* hotkeySettingsGroup = new QGroupBox("热键设置", m_settingsTab);
    QVBoxLayout* hotkeySettingsLayout = new QVBoxLayout(hotkeySettingsGroup);

    QHBoxLayout* hotkeyDisplayLayout = new QHBoxLayout();
    m_currentHotkeyTitleLabel = new QLabel("当前管理员热键:", hotkeySettingsGroup);
    m_currentHotkeyDisplayLabel = new QLabel("[" + tr("未设置或加载中...") + "]", hotkeySettingsGroup);
    m_currentHotkeyDisplayLabel->setStyleSheet("font-weight: bold; padding: 2px 5px; background-color: white; border: 1px solid #CFD8DC; border-radius: 3px;");
    m_editHotkeyButton = new QPushButton("修改热键...", hotkeySettingsGroup);
    connect(m_editHotkeyButton, &QPushButton::clicked, this, &AdminDashboardView::onChangeHotkeyClicked);
    hotkeyDisplayLayout->addWidget(m_currentHotkeyTitleLabel);
    hotkeyDisplayLayout->addWidget(m_currentHotkeyDisplayLabel, 1);
    hotkeyDisplayLayout->addWidget(m_editHotkeyButton);
    hotkeySettingsLayout->addLayout(hotkeyDisplayLayout);
    settingsTabLayout->addWidget(hotkeySettingsGroup);

    // --- Password Settings Group ---
    QGroupBox* passwordSettingsGroup = new QGroupBox("密码设置", m_settingsTab);
    QFormLayout* passwordSettingsFormLayout = new QFormLayout(passwordSettingsGroup); // QFormLayout is good for label-field pairs
    passwordSettingsFormLayout->setSpacing(10);
    passwordSettingsFormLayout->setContentsMargins(10,10,10,10);

    m_currentPasswordLabel = new QLabel("当前密码:", passwordSettingsGroup);
    m_currentPasswordLineEdit = new QLineEdit(passwordSettingsGroup);
    m_currentPasswordLineEdit->setEchoMode(QLineEdit::Password);
    passwordSettingsFormLayout->addRow(m_currentPasswordLabel, m_currentPasswordLineEdit);

    m_newPasswordLabel = new QLabel("新密码:", passwordSettingsGroup);
    m_newPasswordLineEdit = new QLineEdit(passwordSettingsGroup);
    m_newPasswordLineEdit->setEchoMode(QLineEdit::Password);
    passwordSettingsFormLayout->addRow(m_newPasswordLabel, m_newPasswordLineEdit);

    m_confirmPasswordLabel = new QLabel("确认新密码:", passwordSettingsGroup);
    m_confirmPasswordLineEdit = new QLineEdit(passwordSettingsGroup);
    m_confirmPasswordLineEdit->setEchoMode(QLineEdit::Password);
    passwordSettingsFormLayout->addRow(m_confirmPasswordLabel, m_confirmPasswordLineEdit);
    
    m_confirmChangePasswordButton = new QPushButton("确认修改密码", passwordSettingsGroup);
    connect(m_confirmChangePasswordButton, &QPushButton::clicked, this, &AdminDashboardView::onConfirmPasswordChangeClicked);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout(); // To align button to the right
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_confirmChangePasswordButton);
    passwordSettingsFormLayout->addRow(buttonLayout); // Add the button layout as a row
    
    settingsTabLayout->addWidget(passwordSettingsGroup);

    // 探测等待时间设置组
    QGroupBox* detectionWaitGroup = new QGroupBox("探测等待时间设置", m_settingsTab);
    QHBoxLayout* detectionWaitLayout = new QHBoxLayout(detectionWaitGroup);
    QLabel* detectionWaitLabel = new QLabel("探测等待时间 (毫秒):", detectionWaitGroup);
    m_detectionWaitMsSpinBox = new QSpinBox(detectionWaitGroup);
    m_detectionWaitMsSpinBox->setRange(1000, 60000);
    m_detectionWaitMsSpinBox->setSingleStep(1000);
    m_detectionWaitMsSpinBox->setSuffix(" ms");
    // 读取config.json初始化
    QFile configFile("config.json");
    int waitMs = 10000;
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detection_wait_ms") && obj["detection_wait_ms"].isDouble()) {
                waitMs = obj["detection_wait_ms"].toInt();
            }
        }
    }
    m_detectionWaitMsSpinBox->setValue(waitMs);
    m_saveDetectionWaitMsButton = new QPushButton("保存", detectionWaitGroup);
    connect(m_saveDetectionWaitMsButton, &QPushButton::clicked, this, &AdminDashboardView::onDetectionWaitMsSaveClicked);
    detectionWaitLayout->addWidget(detectionWaitLabel);
    detectionWaitLayout->addWidget(m_detectionWaitMsSpinBox);
    detectionWaitLayout->addWidget(m_saveDetectionWaitMsButton);
    detectionWaitGroup->setLayout(detectionWaitLayout);
    settingsTabLayout->addWidget(detectionWaitGroup);

    // --- 新增：开机自启动设置 ---
    m_autoStartCheckBox = new QCheckBox("开机自启动", m_settingsTab);
    settingsTabLayout->addWidget(m_autoStartCheckBox);
    connect(m_autoStartCheckBox, &QCheckBox::toggled, this, &AdminDashboardView::onAutoStartCheckBoxToggled);
    updateAutoStartCheckBoxState(); // 启动时同步状态
    // --- END ---

    // --- 新增：置顶策略设置 ---
    QGroupBox* topmostSettingsGroup = new QGroupBox("窗口置顶策略", m_settingsTab);
    QVBoxLayout* topmostSettingsLayout = new QVBoxLayout(topmostSettingsGroup);
    m_smartTopmostCheckBox = new QCheckBox("智能置顶（推荐，自动检测Z序并适时置顶）", topmostSettingsGroup);
    m_forceTopmostCheckBox = new QCheckBox("强力置顶（持续定时强制置顶，适用于特殊抢焦点应用，可能影响系统弹窗）", topmostSettingsGroup);
    QLabel* topmostTipLabel = new QLabel("说明：\n- 智能置顶：仅在检测到应用被覆盖时自动恢复置顶，兼顾体验和兼容性。\n- 强力置顶：每隔1秒强制将应用窗口置顶，适合极端场景，但可能导致输入法、弹窗等被遮挡。\n- 两者可同时开启，强力置顶优先生效。", topmostSettingsGroup);
    topmostTipLabel->setWordWrap(true);
    topmostSettingsLayout->addWidget(m_smartTopmostCheckBox);
    topmostSettingsLayout->addWidget(m_forceTopmostCheckBox);
    topmostSettingsLayout->addWidget(topmostTipLabel);
    topmostSettingsGroup->setLayout(topmostSettingsLayout);
    settingsTabLayout->addWidget(topmostSettingsGroup);
    // 信号连接
    connect(m_smartTopmostCheckBox, &QCheckBox::toggled, this, &AdminDashboardView::onSmartTopmostCheckBoxToggled);
    connect(m_forceTopmostCheckBox, &QCheckBox::toggled, this, &AdminDashboardView::onForceTopmostCheckBoxToggled);
    // 加载配置文件，设置CheckBox初始状态
    updateTopmostCheckBoxState();

    settingsTabLayout->addStretch(1); 
    m_settingsTab->setLayout(settingsTabLayout);
    m_tabWidget->addTab(m_settingsTab, "系统设置");

    // Add TabWidget to the main layout, making it the central expanding part
    m_mainLayout->addWidget(m_tabWidget, 1); // The '1' stretch factor is important

    // --- Bottom Buttons Layout ---
    m_exitButton = new QPushButton("关闭仪表盘 (返回用户模式)", this);
    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "AdminDashboardView: '关闭仪表盘' (m_exitButton) CLICKED internally.";
        qDebug() << "管理员仪表盘(AdminDashboardView): '关闭仪表盘'按钮被点击。";
        emit userRequestsExitAdminMode();
    });

    m_exitApplicationButton = new QPushButton("退出整个程序", this);
    m_exitApplicationButton->setStyleSheet(
        "QPushButton {" \
        "    background-color: #D32F2F; /* Red 700 */" \
        "    color: white;" \
        "    border: none;" \
        "    padding: 8px 16px;" \
        "    font-size: 13px;" \
        "    border-radius: 4px;" \
        "    text-transform: uppercase;" \
        "}" \
        "QPushButton:hover {" \
        "    background-color: #C62828; /* Red 800 */" \
        "}" \
        "QPushButton:pressed {" \
        "    background-color: #B71C1C; /* Red 900 */" \
        "}"
    );
    connect(m_exitApplicationButton, &QPushButton::clicked, this, &AdminDashboardView::onExitApplicationClicked);
    
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_exitButton);
    bottomLayout->addSpacing(10);
    bottomLayout->addWidget(m_exitApplicationButton);
    
    // Add bottom buttons layout to the main layout, it will not expand
    m_mainLayout->addLayout(bottomLayout);
    
    setLayout(m_mainLayout);
}

void AdminDashboardView::setWhitelistedApps(const QList<AppInfo>& apps)
{
    qDebug() << "AdminDashboardView::setWhitelistedApps called with" << apps.count() << "apps.";
    m_currentApps = apps;
    populateWhitelistView();
}

void AdminDashboardView::populateWhitelistView()
{
    if (!m_whitelistListWidget) return;
    m_whitelistListWidget->clear();
    for (int i = 0; i < m_currentApps.size(); ++i) {
        const AppInfo& app = m_currentApps[i];
        QString itemText = QString("%1 (%2)").arg(app.name).arg(app.path);
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, app.path); // Store path for easy retrieval
        item->setToolTip(itemText); // 设置工具提示显示完整文本
        if (!app.icon.isNull()) {
            item->setIcon(app.icon);
        }
        // 创建自定义小部件，包含两个QCheckBox
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0,0,0,0);
        QLabel* nameLabel = new QLabel(itemText, widget);
        QCheckBox* smartBox = new QCheckBox("智能置顶", widget);
        QCheckBox* forceBox = new QCheckBox("强力置顶", widget);
        smartBox->setChecked(app.smartTopmost);
        forceBox->setChecked(app.forceTopmost);
        // 绑定槽函数，捕获当前索引
        connect(smartBox, &QCheckBox::toggled, this, [this, i, smartBox, forceBox](bool checked){
            onAppTopmostCheckBoxChanged(i, checked, forceBox->isChecked());
        });
        connect(forceBox, &QCheckBox::toggled, this, [this, i, smartBox, forceBox](bool checked){
            onAppTopmostCheckBoxChanged(i, smartBox->isChecked(), checked);
        });
        layout->addWidget(nameLabel);
        layout->addWidget(smartBox);
        layout->addWidget(forceBox);
        layout->addStretch();
        widget->setLayout(layout);
        m_whitelistListWidget->addItem(item);
        m_whitelistListWidget->setItemWidget(item, widget);
    }
}

void AdminDashboardView::setCurrentAdminLoginHotkey(const QStringList& hotkeyStrings)
{
    if (m_currentHotkeyDisplayLabel) {
        if (hotkeyStrings.isEmpty()) {
            m_currentHotkeyDisplayLabel->setText("[" + tr("未设置") + "]");
        } else {
            m_currentHotkeyDisplayLabel->setText(hotkeyStrings.join(" + "));
        }
        qDebug() << "AdminDashboardView: Current admin hotkey display updated to:" << m_currentHotkeyDisplayLabel->text();
    } else {
        qWarning() << "AdminDashboardView: m_currentHotkeyDisplayLabel is null, cannot set hotkey string.";
    }
}

void AdminDashboardView::onChangeHotkeyClicked()
{
    qDebug() << "AdminDashboardView::onChangeHotkeyClicked INVOKED.";
    qDebug() << "AdminDashboardView: '修改热键' button clicked.";
    HotkeyEditDialog hotkeyDialog(this);
    // Potentially set existing hotkey in dialog if it supports it (it doesn't seem to currently)
    
    if (hotkeyDialog.exec() == QDialog::Accepted) {
        QList<DWORD> newVkCodes = hotkeyDialog.getSelectedHotkeyVkCodes();
        QStringList newVkStrings = hotkeyDialog.getSelectedHotkey(); // For display and confirmation

        if (newVkCodes.isEmpty() || newVkStrings.isEmpty()) {
            qWarning() << "AdminDashboardView: Hotkey dialog accepted, but returned empty hotkey sequence.";
            QMessageBox::warning(this, tr("热键无效"), tr("未能获取有效的热键组合。"));
            return;
        }

        qDebug() << "AdminDashboardView: New hotkey selected (VK_Strings):" << newVkStrings.join(" + ");
        qDebug() << "AdminDashboardView: New hotkey selected (VK_Codes):" << newVkCodes;
        
        // Update the display label immediately
        if (m_currentHotkeyDisplayLabel) {
            m_currentHotkeyDisplayLabel->setText(newVkStrings.join(" + "));
        }
        
        emit adminLoginHotkeyChanged(newVkCodes);
    } else {
        qDebug() << "AdminDashboardView: Hotkey dialog cancelled or closed.";
    }
}

void AdminDashboardView::onAddAppClicked()
{
    qDebug() << "AdminDashboardView::onAddAppClicked INVOKED.";
    qDebug() << "AdminDashboardView: Add App button clicked.";
    QString appPath = QFileDialog::getOpenFileName(
        this,
        "选择应用程序",
        QString(), // Default directory
        "可执行文件 (*.exe);;所有文件 (*.*)"
    );

    if (appPath.isEmpty()) {
        return; // User cancelled
    }

    QFileInfo fileInfo(appPath);
    QString appName = QInputDialog::getText(
        this,
        "输入应用名称",
        "应用名称:",
        QLineEdit::Normal,
        fileInfo.completeBaseName() // Suggest base name
    );

    if (appName.isEmpty()) {
        return; // User cancelled or entered empty name
    }

    // Check for duplicates (by path)
    for (const AppInfo& existingApp : m_currentApps) {
        if (existingApp.path.compare(appPath, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, "重复应用", "该应用程序已在白名单中。");
            return;
        }
    }

    AppInfo newApp;
    newApp.name = appName;
    newApp.path = appPath;
    // 自动补全 mainExecutableHint，确保后续状态栏能正确探测进程
    QFileInfo fi(appPath);
    newApp.mainExecutableHint = fi.fileName(); // 以文件名作为进程名Hint
    // Try to get icon - for now, this might be a basic attempt or rely on AdminModule later
    // For immediate UI update, we might need a temporary icon or SystemInteractionModule direct call if safe.
    // Let's assume icon loading is handled when the list is fully processed by AdminModule/UserModeModule.
    // For this local view, we might not load it here, or just try a basic QIcon(appPath).
    newApp.icon = QIcon(appPath); // Basic icon load attempt for the list view
    if (newApp.icon.isNull()) {
        qWarning() << "AdminDashboardView: Could not load icon for" << appPath << "during add.";
    }

    // 在populateWhitelistView()中，为每个应用条目动态添加QWidget（含两个QCheckBox），并与AppInfo的smartTopmost/forceTopmost字段联动。勾选变化时，更新m_currentApps并emit whitelistChanged。
    // 在onAddAppClicked和onDetectionDialogApplied中，弹窗增加两个QCheckBox，用户可选择置顶策略，保存到newApp.smartTopmost/forceTopmost。
    newApp.smartTopmost = m_smartTopmostCheckBox->isChecked();
    newApp.forceTopmost = m_forceTopmostCheckBox->isChecked();

    m_currentApps.append(newApp);
    populateWhitelistView(); // Refresh the list view

    qDebug() << "AdminDashboardView: Emitting whitelistChanged with" << m_currentApps.count() << "apps.";
    emit whitelistChanged(m_currentApps);
}

void AdminDashboardView::onRemoveAppClicked()
{
    qDebug() << "AdminDashboardView::onRemoveAppClicked INVOKED.";
    qDebug() << "AdminDashboardView: Remove App button clicked.";
    QListWidgetItem* selectedItem = m_whitelistListWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::information(this, "提示", "请先选择一个要移除的应用程序。");
        return;
    }

    QString appPathToRemove = selectedItem->data(Qt::UserRole).toString();
    
    for (int i = 0; i < m_currentApps.size(); ++i) {
        if (m_currentApps[i].path == appPathToRemove) {
            m_currentApps.removeAt(i);
            break;
        }
    }

    populateWhitelistView(); // Refresh the list view

    qDebug() << "AdminDashboardView: Emitting whitelistChanged with" << m_currentApps.count() << "apps.";
    emit whitelistChanged(m_currentApps);
}

void AdminDashboardView::onExitApplicationClicked() {
    qDebug() << "AdminDashboardView::onExitApplicationClicked INVOKED.";
    qDebug() << "管理员仪表盘(AdminDashboardView): '退出整个程序' 按钮被点击。应用程序将退出。";
    // 主动卸载键盘钩子
    if (m_systemInteractionModulePtr) {
        m_systemInteractionModulePtr->uninstallKeyboardHook();
    }
    // 通知UserModeModule终止所有子进程（如有）
    if (m_userModeModule) {
        m_userModeModule->terminateActiveProcesses();
    }
    // 退出应用
    QCoreApplication::instance()->quit();
}

void AdminDashboardView::onConfirmPasswordChangeClicked()
{
    qDebug() << "AdminDashboardView::onConfirmPasswordChangeClicked INVOKED.";

    QString currentPassword = m_currentPasswordLineEdit->text();
    QString newPassword = m_newPasswordLineEdit->text();
    QString confirmPassword = m_confirmPasswordLineEdit->text();

    if (currentPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("所有密码字段都必须填写。"));
        return;
    }

    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, tr("密码不匹配"), tr("新密码和确认新密码字段不一致。"));
        m_newPasswordLineEdit->clear();
        m_confirmPasswordLineEdit->clear();
        m_newPasswordLineEdit->setFocus();
        return;
    }
    
    // Basic complexity check: e.g., minimum length
    if (newPassword.length() < 6) {
        QMessageBox::warning(this, tr("密码太短"), tr("新密码长度至少需要6个字符。"));
        m_newPasswordLineEdit->clear();
        m_confirmPasswordLineEdit->clear();
        m_newPasswordLineEdit->setFocus();
        return;
    }
    
    // Optional: Check if new password is the same as current password (usually not allowed)
    // This would require AdminModule to verify currentPassword first, then compare.
    // For now, we just emit. If AdminModule finds currentPassword is wrong, it will handle it.

    qDebug() << "AdminDashboardView: Emitting changePasswordRequested.";
    emit changePasswordRequested(currentPassword, newPassword);

    // Clear fields after attempting (regardless of success at AdminModule level, for security)
    m_currentPasswordLineEdit->clear();
    m_newPasswordLineEdit->clear();
    m_confirmPasswordLineEdit->clear();
    
    // Optionally, provide feedback that request was sent, actual success depends on AdminModule
    qDebug() << "AdminDashboardView::onConfirmPasswordChangeClicked INVOKED.";
    qDebug() << "管理员仪表盘(AdminDashboardView): '修改密码' 按钮被点击。";
}

void AdminDashboardView::onDetectAndAddAppClicked()
{
    if (!m_systemInteractionModulePtr) {
        QMessageBox::critical(this, "错误", "系统交互模块不可用。");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择应用程序可执行文件"),
        QString(), // Default directory
        tr("可执行文件 (*.exe);;所有文件 (*)")
    );

    if (filePath.isEmpty()) {
        return; // User cancelled
    }

    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName(); // Suggest app name based on file name

    bool ok;
    QString appName = QInputDialog::getText(
        this,
        tr("输入应用名称"),
        tr("应用名称:"),
        QLineEdit::Normal,
        baseName, // Default to executable base name
        &ok
    );

    if (!ok || appName.isEmpty()) {
        return; // User cancelled or entered empty name
    }

    m_pendingDetectionAppPath = filePath;
    m_pendingDetectionAppName = appName;

    // 新增：显示探测进度弹窗
    if (!m_detectionProgressDialog) {
        m_detectionProgressDialog = new QProgressDialog("正在探测应用主窗口，请稍候...", nullptr, 0, 0, this);
        m_detectionProgressDialog->setWindowModality(Qt::WindowModal);
        m_detectionProgressDialog->setCancelButton(nullptr);
        m_detectionProgressDialog->setMinimumDuration(0);
        m_detectionProgressDialog->setWindowTitle("探测进度");
    }
    m_detectionProgressDialog->show();

    m_detectAndAddAppButton->setEnabled(false);
    m_detectAndAddAppButton->setText("正在探测...");
    qDebug() << "[AdminDashboardView] Requesting detection for:" << filePath << "App Name:" << appName;
    m_systemInteractionModulePtr->startExecutableDetection(filePath, appName);
}

void AdminDashboardView::onDetectionResultsReceived(const SuggestedWindowHints& hints, bool success, const QString& errorString)
{
    // 探测完成后关闭进度弹窗
    if (m_detectionProgressDialog) {
        m_detectionProgressDialog->hide();
    }
    m_detectAndAddAppButton->setText("探测并添加应用...");
    m_detectAndAddAppButton->setEnabled(true);

    if (!success) {
        // 新增：获取候选窗口信息并传递给弹窗
        QList<WindowCandidateInfo> candidates = m_systemInteractionModulePtr->getLastDetectionCandidates();
        DetectionResultDialog dialog(hints, this);
        disconnect(&dialog, &DetectionResultDialog::suggestionsApplied, this, &AdminDashboardView::onDetectionDialogApplied);
        connect(&dialog, &DetectionResultDialog::suggestionsApplied, this, &AdminDashboardView::onDetectionDialogApplied);
        if (dialog.exec() == QDialog::Accepted) {
            qDebug() << "[AdminDashboardView] DetectionResultDialog accepted by user.";
        } else {
            qDebug() << "[AdminDashboardView] DetectionResultDialog cancelled by user.";
            m_pendingDetectionAppPath.clear();
            m_pendingDetectionAppName.clear();
        }
        return;
    }

    qDebug() << "[AdminDashboardView] Showing DetectionResultDialog for:" << m_pendingDetectionAppName;
    DetectionResultDialog dialog(hints, this);
    disconnect(&dialog, &DetectionResultDialog::suggestionsApplied, this, &AdminDashboardView::onDetectionDialogApplied);
    connect(&dialog, &DetectionResultDialog::suggestionsApplied, this, &AdminDashboardView::onDetectionDialogApplied);
    if (dialog.exec() == QDialog::Accepted) {
        qDebug() << "[AdminDashboardView] DetectionResultDialog accepted by user.";
    } else {
        qDebug() << "[AdminDashboardView] DetectionResultDialog cancelled by user.";
        m_pendingDetectionAppPath.clear();
        m_pendingDetectionAppName.clear();
    }
}

void AdminDashboardView::onDetectionDialogApplied(const QString& finalMainExecutableHint, const QJsonObject& finalWindowHints)
{
    qDebug() << "AdminDashboardView::onDetectionDialogApplied - AppPath:" << m_pendingDetectionAppPath
             << "AppName:" << m_pendingDetectionAppName
             << "MainExeHint:" << finalMainExecutableHint
             << "WindowHints:" << finalWindowHints.keys();

    if (m_pendingDetectionAppPath.isEmpty() || m_pendingDetectionAppName.isEmpty()) {
        qWarning() << "AdminDashboardView: Pending detection app path or name is empty. Cannot add app.";
        QMessageBox::warning(this, "添加失败", "无法添加应用，因为原始应用路径或名称丢失。");
        return;
    }

    // Check for duplicates before adding
    for (const AppInfo& existingApp : qAsConst(m_currentApps)) {
        if (existingApp.path == m_pendingDetectionAppPath) {
            QMessageBox::warning(this, "重复应用", "此应用程序已在白名单中。");
            return;
        }
    }

    AppInfo newApp;
    newApp.name = m_pendingDetectionAppName; // Use the initially proposed name
    newApp.path = m_pendingDetectionAppPath; // CRITICAL: Use the stored full path
    newApp.mainExecutableHint = finalMainExecutableHint;
    newApp.windowFindingHints = finalWindowHints;

    // --- BEGIN MODIFICATION: Get and set the icon ---
    if (m_systemInteractionModulePtr && !m_pendingDetectionAppPath.isEmpty()) {
        newApp.icon = m_systemInteractionModulePtr->getIconForExecutable(m_pendingDetectionAppPath);
        if (newApp.icon.isNull()) {
            qWarning() << "AdminDashboardView: Failed to get icon for" << m_pendingDetectionAppPath << ". Using default icon.";
            // Optionally set a default icon here if desired, e.g.:
            // newApp.icon = QApplication::style()->standardIcon(QStyle::SP_ExecutableFile);
        }
    } else {
        qWarning() << "AdminDashboardView: SystemInteractionModule is null or pending app path is empty, cannot get icon. App will have no icon.";
    }
    // --- END MODIFICATION ---

    // 在populateWhitelistView()中，为每个应用条目动态添加QWidget（含两个QCheckBox），并与AppInfo的smartTopmost/forceTopmost字段联动。勾选变化时，更新m_currentApps并emit whitelistChanged。
    // 在onAddAppClicked和onDetectionDialogApplied中，弹窗增加两个QCheckBox，用户可选择置顶策略，保存到newApp.smartTopmost/forceTopmost。
    newApp.smartTopmost = m_smartTopmostCheckBox->isChecked();
    newApp.forceTopmost = m_forceTopmostCheckBox->isChecked();

    m_currentApps.append(newApp);
    populateWhitelistView(); // Refresh the list widget directly

    qDebug() << "AdminDashboardView: App" << newApp.name << "added/updated. Emitting whitelistChanged.";
    emit whitelistChanged(m_currentApps);

    // Clear pending info
    m_pendingDetectionAppPath.clear();
    m_pendingDetectionAppName.clear();
}

void AdminDashboardView::onDetectionWaitMsSaveClicked() {
    int newWaitMs = m_detectionWaitMsSpinBox->value();
    QFile configFile("config.json");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法打开配置文件");
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &err);
    configFile.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "保存失败", "配置文件格式错误");
        return;
    }
    QJsonObject obj = doc.object();
    obj["detection_wait_ms"] = newWaitMs;
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "保存失败", "无法写入配置文件");
        return;
    }
    configFile.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    configFile.close();
    QMessageBox::information(this, "保存成功", "探测等待时间已保存");
}

void AdminDashboardView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap bg(":/images/admin_bg.jpg");
    if (!bg.isNull()) {
        painter.drawPixmap(this->rect(), bg);
    } else {
        painter.fillRect(this->rect(), QColor(236, 239, 241)); // Fallback background
    }
}

// 辅助函数：检测当前是否已设置自启动，并同步复选框
void AdminDashboardView::updateAutoStartCheckBoxState() {
    // 注册表路径：HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString exePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    QVariant val = reg.value(appName);
    if (val.isValid() && val.toString() == exePath) {
        m_autoStartCheckBox->setChecked(true);
    } else {
        m_autoStartCheckBox->setChecked(false);
    }
}

// 槽函数：设置或取消自启动
void AdminDashboardView::onAutoStartCheckBoxToggled(bool checked) {
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString exePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    if (checked) {
        reg.setValue(appName, exePath);
        qDebug() << "已设置开机自启动:" << exePath;
    } else {
        reg.remove(appName);
        qDebug() << "已取消开机自启动:" << exePath;
    }
}

// 辅助函数：检测当前是否已设置置顶策略，并同步复选框
void AdminDashboardView::updateTopmostCheckBoxState() {
    // 注册表路径：HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString exePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    QVariant val = reg.value(appName);
    if (val.isValid() && val.toString() == exePath) {
        m_smartTopmostCheckBox->setChecked(true);
        m_forceTopmostCheckBox->setChecked(true);
    } else {
        m_smartTopmostCheckBox->setChecked(false);
        m_forceTopmostCheckBox->setChecked(false);
    }
}

// 槽函数：设置或取消智能置顶
void AdminDashboardView::onSmartTopmostCheckBoxToggled(bool checked) {
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString exePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    if (checked) {
        reg.setValue(appName, exePath);
        qDebug() << "已设置智能置顶:" << exePath;
    } else {
        reg.remove(appName);
        qDebug() << "已取消智能置顶:" << exePath;
    }
}

// 槽函数：设置或取消强力置顶
void AdminDashboardView::onForceTopmostCheckBoxToggled(bool checked) {
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appName = QCoreApplication::applicationName();
    QString exePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    if (checked) {
        reg.setValue(appName, exePath);
        qDebug() << "已设置强力置顶:" << exePath;
    } else {
        reg.remove(appName);
        qDebug() << "已取消强力置顶:" << exePath;
    }
}

// 置顶策略复选框联动槽函数实现
// 参数说明：appIndex-应用索引，smartChecked-智能置顶状态，forceChecked-强力置顶状态
void AdminDashboardView::onAppTopmostCheckBoxChanged(int appIndex, bool smartChecked, bool forceChecked)
{
    // 边界检查，防止越界
    if (appIndex < 0 || appIndex >= m_currentApps.size()) return;

    // 更新当前应用的置顶策略
    m_currentApps[appIndex].smartTopmost = smartChecked;
    m_currentApps[appIndex].forceTopmost = forceChecked;

    // 立即发射白名单变更信号，保证配置和UI同步
    emit whitelistChanged(m_currentApps);

    // 可选：调试输出
    qDebug() << "AdminDashboardView: 应用" << m_currentApps[appIndex].name
             << "置顶策略已更新，智能置顶:" << smartChecked << "强力置顶:" << forceChecked;
}

// Implement other methods and slots as functionality is migrated 