#include "WhitelistManagerView.h"
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>
#include <QHideEvent>
#include <QCoreApplication>
#include <QGroupBox>
#include <QFormLayout>
#include "HotkeyEditDialog.h"
#include "common_types.h"

WhitelistManagerView::WhitelistManagerView(const QList<AppInfo>& currentWhitelist, QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setWindowTitle("白名单应用管理");
    setMinimumSize(450, 300);
    populateList(currentWhitelist);
    qDebug() << "白名单管理视图(WhitelistManagerView): 已创建。";
}

WhitelistManagerView::~WhitelistManagerView()
{
    qDebug() << "白名单管理视图(WhitelistManagerView): 已销毁。";
}

void WhitelistManagerView::setupUi()
{
    m_appListWidget = new QListWidget(this);
    m_appListWidget->setToolTip("当前白名单中的应用程序列表。");

    m_addButton = new QPushButton("添加应用", this);
    m_addButton->setToolTip("添加一个新的应用程序到白名单。");
    connect(m_addButton, &QPushButton::clicked, this, &WhitelistManagerView::onAddAppClicked);

    m_removeButton = new QPushButton("移除应用", this);
    m_removeButton->setToolTip("从白名单中移除选定的应用程序。");
    connect(m_removeButton, &QPushButton::clicked, this, &WhitelistManagerView::onRemoveAppClicked);

    QPushButton *exitAdminModeButton = new QPushButton("完成并退出管理", this);
    exitAdminModeButton->setToolTip("关闭管理界面并返回主模式。");
    connect(exitAdminModeButton, &QPushButton::clicked, this, &WhitelistManagerView::userRequestsExitAdminMode);

    m_secureExitProgramButton = new QPushButton("安全退出程序", this);
    m_secureExitProgramButton->setToolTip("关闭整个剑鞘系统应用程序。");
    connect(m_secureExitProgramButton, &QPushButton::clicked, []() {
        qDebug() << "白名单管理视图(WhitelistManagerView): '安全退出程序'按钮被点击。程序将退出。";
        QCoreApplication::instance()->quit();
    });

    m_editHotkeyButton = new QPushButton("修改登录热键", this);
    m_editHotkeyButton->setToolTip("修改用于进入管理员登录界面的热键组合。");
    connect(m_editHotkeyButton, &QPushButton::clicked, this, &WhitelistManagerView::onEditHotkeyClicked);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(exitAdminModeButton);
    buttonLayout->addWidget(m_secureExitProgramButton);
    buttonLayout->addWidget(m_editHotkeyButton);

    // --- Password Change UI --- 
    m_passwordChangeGroupBox = new QGroupBox("修改管理员密码", this);
    QFormLayout *passwordFormLayout = new QFormLayout;
    m_currentPasswordEdit = new QLineEdit(this);
    m_currentPasswordEdit->setEchoMode(QLineEdit::Password);
    m_currentPasswordEdit->setPlaceholderText("输入当前密码");
    passwordFormLayout->addRow("当前密码:", m_currentPasswordEdit);

    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setPlaceholderText("输入新密码");
    passwordFormLayout->addRow("新密码:", m_newPasswordEdit);

    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setPlaceholderText("确认新密码");
    passwordFormLayout->addRow("确认新密码:", m_confirmPasswordEdit);

    m_changePasswordButton = new QPushButton("确认修改密码", this);
    connect(m_changePasswordButton, &QPushButton::clicked, this, &WhitelistManagerView::onChangePasswordClicked);
    passwordFormLayout->addWidget(m_changePasswordButton);
    
    m_passwordChangeGroupBox->setLayout(passwordFormLayout);
    // --- End Password Change UI ---

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_appListWidget);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_passwordChangeGroupBox);
    mainLayout->addWidget(m_editHotkeyButton);

    setLayout(mainLayout);

    // Style (optional, can be refined with QSS later)
    setStyleSheet("QPushButton { padding: 5px 10px; } QListWidget { font-size: 14px; }");
}

void WhitelistManagerView::populateList(const QList<AppInfo>& apps)
{
    m_appListWidget->clear();
    for (const AppInfo& app : apps)
    {
        QListWidgetItem *item = new QListWidgetItem(app.name);
        item->setData(Qt::UserRole, app.path); // Store path in UserRole
        if (!app.icon.isNull()) {
            item->setIcon(app.icon);
        }
        m_appListWidget->addItem(item);
    }
    qDebug() << "白名单管理视图: 列表已使用" << apps.size() << "个应用填充。";
}

QList<AppInfo> WhitelistManagerView::getCurrentApps() const
{
    QList<AppInfo> apps;
    for (int i = 0; i < m_appListWidget->count(); ++i)
    {
        QListWidgetItem *item = m_appListWidget->item(i);
        AppInfo app;
        app.name = item->text();
        app.path = item->data(Qt::UserRole).toString();
        app.icon = item->icon(); // Retrieve icon directly from item
        apps.append(app);
    }
    return apps;
}

void WhitelistManagerView::onAddAppClicked()
{
    QString appPath = QFileDialog::getOpenFileName(
        this,
        "选择应用程序可执行文件",
        QString(), // Default directory
        "可执行文件 (*.exe);;所有文件 (*)"
    );

    if (appPath.isEmpty()) {
        return; // User cancelled
    }

    QFileInfo fileInfo(appPath);
    QString defaultName = fileInfo.completeBaseName(); // Get name without extension

    bool ok;
    QString appName = QInputDialog::getText(
        this,
        "输入应用名称",
        "为此应用程序指定一个名称:",
        QLineEdit::Normal,
        defaultName,
        &ok
    );

    if (ok && !appName.isEmpty()) {
        // Check for duplicates (path-based)
        for (int i = 0; i < m_appListWidget->count(); ++i) {
            if (m_appListWidget->item(i)->data(Qt::UserRole).toString() == appPath) {
                QMessageBox::warning(this, "重复应用", "此应用程序已在白名单中。");
                return;
            }
        }

        AppInfo newApp = {appName, appPath};
        QListWidgetItem *item = new QListWidgetItem(appName);
        item->setData(Qt::UserRole, appPath);
        if (!newApp.icon.isNull()) {
            item->setIcon(newApp.icon);
        }
        m_appListWidget->addItem(item);
        qDebug() << "白名单管理视图: 已添加应用 - 名称:" << newApp.name << "路径:" << newApp.path;
        emit whitelistChanged(getCurrentApps());
    }
}

void WhitelistManagerView::onRemoveAppClicked()
{
    QListWidgetItem *selectedItem = m_appListWidget->currentItem();
    if (!selectedItem)
    {
        QMessageBox::information(this, "提示", "请先选择一个要移除的应用程序。");
        return;
    }

    int ret = QMessageBox::question(this, "确认移除", 
                                    QString("您确定要从白名单中移除 \"%1\" 吗?").arg(selectedItem->text()),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QString removedAppName = selectedItem->text();
        QString removedAppPath = selectedItem->data(Qt::UserRole).toString();
        delete m_appListWidget->takeItem(m_appListWidget->row(selectedItem)); // takeItem and delete
        qDebug() << "白名单管理视图: 已移除应用 - 名称:" << removedAppName << "路径:" << removedAppPath;
        emit whitelistChanged(getCurrentApps());
    }
}

void WhitelistManagerView::onEditHotkeyClicked()
{
    qDebug() << "白名单管理视图: '修改登录热键'按钮被点击。";
    HotkeyEditDialog dialog(this);
    connect(&dialog, &HotkeyEditDialog::hotkeySelected, this, &WhitelistManagerView::handleNewAdminHotkeySelected);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Hotkey selection handled by the signal/slot connection
        qDebug() << "白名单管理视图: HotkeyEditDialog 已接受。新的热键 (如果有效) 将通过信号发送。";
    } else {
        qDebug() << "白名单管理视图: HotkeyEditDialog 已取消。";
    }
}

void WhitelistManagerView::handleNewAdminHotkeySelected(const QStringList& newVkStrings)
{
    if (newVkStrings.isEmpty()) {
        qDebug() << "白名单管理视图: 收到空的热键序列，不进行更新。";
        return;
    }
    qDebug() << "白名单管理视图: 收到新的热键序列:" << newVkStrings.join(" + ");
    emit adminLoginHotkeyChanged(newVkStrings);
    QMessageBox::information(this, "热键已更新", 
                             QString("新的管理员登录热键已设置为: %1\n请注意，此更改将在下次程序启动时生效。").arg(newVkStrings.join(" + ")));
}

void WhitelistManagerView::onChangePasswordClicked()
{
    QString currentPassword = m_currentPasswordEdit->text();
    QString newPassword = m_newPasswordEdit->text();
    QString confirmPassword = m_confirmPasswordEdit->text();

    if (newPassword.isEmpty()) {
        QMessageBox::warning(this, "密码为空", "新密码不能为空。");
        return;
    }
    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, "密码不匹配", "新密码和确认密码不一致。");
        m_newPasswordEdit->clear();
        m_confirmPasswordEdit->clear();
        return;
    }
    // Optionally, add complexity requirements for the new password here

    emit changePasswordRequested(currentPassword, newPassword);

    // Clear fields after attempt
    m_currentPasswordEdit->clear();
    m_newPasswordEdit->clear();
    m_confirmPasswordEdit->clear();
}

// Placeholder for potential future use, like inline editing item names
void WhitelistManagerView::onListItemChanged(QListWidgetItem *item)
{
    if(item){
        //qDebug() << "List item changed: " << item->text();
        // If we allow editing, we might want to update the stored name and emit whitelistChanged
    }
}

// void WhitelistManagerView::hideEvent(QHideEvent *event) // REMOVED hideEvent implementation
// {
//     qDebug() << "白名单管理视图(WhitelistManagerView): Hide event triggered.";
//     emit viewReallyClosed();
//     QWidget::hideEvent(event);
// } 