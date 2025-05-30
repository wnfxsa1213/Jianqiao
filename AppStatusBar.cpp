#include "AppStatusBar.h"
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QToolTip>
#include "SystemInteractionModule.h"
#include <QAction>
#include <QMessageBox>

// 构造函数
AppStatusBar::AppStatusBar(QWidget* parent)
    : QWidget(parent), m_model(nullptr)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(10);
    setLayout(m_layout);
}

// 绑定数据模型
void AppStatusBar::setModel(AppStatusModel* model) {
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
    m_model = model;
    if (m_model) {
        connect(m_model, &AppStatusModel::statusChanged, this, &AppStatusBar::refreshCards);
        refreshCards();
    }
}

// 刷新所有卡片UI
void AppStatusBar::refreshCards() {
    // 彻底清理布局中所有项（包括stretch和widget），防止漂移
    while (QLayoutItem* item = m_layout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater(); // 删除控件
        delete item; // 删除布局项（包括stretch）
    }
    m_cardWidgets.clear();
    if (!m_model) return;
    int count = m_model->rowCount();
    // 先加左侧弹性空间，保证居中
    m_layout->addStretch();
    for (int i = 0; i < count; ++i) {
        const AppStatus status = m_model->getStatus(i);
        QPushButton* btn = new QPushButton(this);
        btn->setIcon(status.icon);
        btn->setIconSize(QSize(32,32));
        btn->setText(status.appName);
        btn->setToolTip(QString("%1\n状态：%2\nPID：%3")
            .arg(status.appName)
            .arg([&]{
                switch(status.status) {
                    case AppRunStatus::NotRunning: return QStringLiteral("未启动");
                    case AppRunStatus::Running: return QStringLiteral("运行中");
                    case AppRunStatus::Minimized: return QStringLiteral("最小化");
                    case AppRunStatus::Activated: return QStringLiteral("激活中");
                    case AppRunStatus::Error: return QStringLiteral("异常");
                }
                return QStringLiteral("未知");
            }())
            .arg(status.pid));
        // 状态高亮优化
        if (status.status == AppRunStatus::Activated) {
            // 激活中：高亮蓝色
            btn->setStyleSheet("background:#cceeff;border:2px solid #3399cc;border-radius:8px;");
        } else if (status.status == AppRunStatus::Minimized) {
            // 最小化：淡黄色高亮
            btn->setStyleSheet("background:#fff7cc;border:2px solid #e6c200;border-radius:8px;");
        } else if (status.status == AppRunStatus::Running) {
            // 运行中：淡蓝色
            btn->setStyleSheet("background:#e6f2ff;border:1px solid #99c2e6;border-radius:8px;");
        } else if (status.status == AppRunStatus::Error) {
            // 异常：红色
            btn->setStyleSheet("background:#ffe0e0;border:2px solid #cc3333;border-radius:8px;");
        } else if (status.status == AppRunStatus::NotRunning) {
            // 未启动：灰色
            btn->setStyleSheet("background:#f0f0f0;color:#aaa;border:1px solid #ccc;border-radius:8px;");
        }
        // 点击信号：激活窗口
        connect(btn, &QPushButton::clicked, this, [this, i, status]() {
            if (status.hwnd && status.status != AppRunStatus::NotRunning) {
                extern SystemInteractionModule* systemInteractionModule;
                if (systemInteractionModule) {
                    systemInteractionModule->activateWindow(status.hwnd);
                }
            }
            emit appCardClicked(i);
        });
        // 右键菜单
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QPushButton::customContextMenuRequested, this, [this, i, status, btn](const QPoint& pos) {
            QMenu menu;
            QAction* actActivate = menu.addAction("激活窗口");
            QAction* actClose = menu.addAction("关闭应用");
            QAction* actRestart = menu.addAction("重启应用");
            QAction* actDetail = menu.addAction("查看详情");
            // 激活窗口
            actActivate->setEnabled(status.hwnd && status.status != AppRunStatus::NotRunning);
            // 关闭/重启仅运行中可用
            actClose->setEnabled(status.pid != 0);
            actRestart->setEnabled(status.pid != 0);
            QAction* sel = menu.exec(btn->mapToGlobal(pos));
            extern SystemInteractionModule* systemInteractionModule;
            if (sel == actActivate && systemInteractionModule && status.hwnd) {
                systemInteractionModule->activateWindow(status.hwnd);
            } else if (sel == actClose && status.pid != 0) {
                // 关闭进程
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, status.pid);
                if (hProc) { TerminateProcess(hProc, 0); CloseHandle(hProc); }
            } else if (sel == actRestart && status.pid != 0) {
                // 关闭后重启
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, status.pid);
                if (hProc) { TerminateProcess(hProc, 0); CloseHandle(hProc); }
                QProcess::startDetached(status.exePath);
            } else if (sel == actDetail) {
                QString info = QString("应用名称：%1\n路径：%2\nPID：%3\n状态：%4")
                    .arg(status.appName).arg(status.exePath).arg(status.pid)
                    .arg([&]{
                        switch(status.status) {
                            case AppRunStatus::NotRunning: return QStringLiteral("未启动");
                            case AppRunStatus::Running: return QStringLiteral("运行中");
                            case AppRunStatus::Minimized: return QStringLiteral("最小化");
                            case AppRunStatus::Activated: return QStringLiteral("激活中");
                            case AppRunStatus::Error: return QStringLiteral("异常");
                        }
                        return QStringLiteral("未知");
                    }());
                QMessageBox::information(this, "应用详情", info);
            }
            emit appCardRightClicked(i, mapToGlobal(pos));
        });
        m_layout->addWidget(btn);
        m_cardWidgets.append(btn);
    }
    // 再加右侧弹性空间，保证居中
    m_layout->addStretch();
} 