#include "AdminDashboardView.h"
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

AdminDashboardView::AdminDashboardView(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_whitelistTab(nullptr)
    , m_settingsTab(nullptr)
    , m_whitelistListWidget(nullptr)
    , m_addAppButton(nullptr)
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
{
    qDebug() << "管理员仪表盘(AdminDashboardView): 已创建。";
    setupUi();
}

AdminDashboardView::~AdminDashboardView()
{
    qDebug() << "管理员仪表盘(AdminDashboardView): 已销毁。";
}

void AdminDashboardView::setupUi()
{
    setWindowTitle("管理员仪表盘");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    whitelistGroupLayout->addWidget(m_whitelistListWidget, 1); // Give list widget stretch factor

    QHBoxLayout* whitelistButtonsLayout = new QHBoxLayout();
    m_addAppButton = new QPushButton("添加应用...", whitelistGroup);
    m_removeAppButton = new QPushButton("移除选中", whitelistGroup);
    connect(m_addAppButton, &QPushButton::clicked, this, &AdminDashboardView::onAddAppClicked);
    connect(m_removeAppButton, &QPushButton::clicked, this, &AdminDashboardView::onRemoveAppClicked);
    whitelistButtonsLayout->addStretch();
    whitelistButtonsLayout->addWidget(m_addAppButton);
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
    for (const AppInfo& app : m_currentApps) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2)").arg(app.name).arg(app.path));
        item->setData(Qt::UserRole, app.path); // Store path for easy retrieval
        if (!app.icon.isNull()) {
            item->setIcon(app.icon);
        }
        m_whitelistListWidget->addItem(item);
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
    // Try to get icon - for now, this might be a basic attempt or rely on AdminModule later
    // For immediate UI update, we might need a temporary icon or SystemInteractionModule direct call if safe.
    // Let's assume icon loading is handled when the list is fully processed by AdminModule/UserModeModule.
    // For this local view, we might not load it here, or just try a basic QIcon(appPath).
    newApp.icon = QIcon(appPath); // Basic icon load attempt for the list view
    if (newApp.icon.isNull()) {
        qWarning() << "AdminDashboardView: Could not load icon for" << appPath << "during add.";
    }


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

// Implement other methods and slots as functionality is migrated 