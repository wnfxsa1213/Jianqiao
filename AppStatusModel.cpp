#include "AppStatusModel.h"
#include <QVariant>

// 构造函数
AppStatusModel::AppStatusModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

// 返回模型行数（即应用数量）
int AppStatusModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_statusList.size();
}

// 返回指定行的数据，支持多种角色
QVariant AppStatusModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_statusList.size())
        return QVariant();
    const AppStatus& status = m_statusList.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return status.appName;
    case Qt::UserRole:
        return status.exePath;
    case Qt::UserRole + 1:
        return QVariant::fromValue(status.icon);
    case Qt::UserRole + 2:
        return static_cast<int>(status.status);
    case Qt::UserRole + 3:
        return static_cast<quint32>(status.pid);
    case Qt::UserRole + 4:
        return reinterpret_cast<quintptr>(status.hwnd);
    case Qt::UserRole + 5:
        return status.lastActive;
    default:
        return QVariant();
    }
}

// 定义所有自定义角色名，便于QML/界面层访问
QHash<int, QByteArray> AppStatusModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "appName";
    roles[Qt::UserRole] = "exePath";
    roles[Qt::UserRole + 1] = "icon";
    roles[Qt::UserRole + 2] = "status";
    roles[Qt::UserRole + 3] = "pid";
    roles[Qt::UserRole + 4] = "hwnd";
    roles[Qt::UserRole + 5] = "lastActive";
    return roles;
}

// 批量刷新所有应用状态
void AppStatusModel::updateStatus(const QList<AppStatus>& statusList) {
    beginResetModel();
    m_statusList = statusList;
    endResetModel();
    emit statusChanged();
}

// 获取单个应用状态
AppStatus AppStatusModel::getStatus(int row) const {
    if (row < 0 || row >= m_statusList.size())
        return AppStatus();
    return m_statusList.at(row);
}

// 获取全部应用状态
QList<AppStatus> AppStatusModel::getAllStatus() const {
    return m_statusList;
}

// ===================== 新增：设置某个应用为激活状态 =====================
void AppStatusModel::setActiveApp(const QString& appPath)
{
    bool found = false;
    for (AppStatus& status : m_statusList) {
        if (status.exePath == appPath) {
            status.status = AppRunStatus::Activated;
            found = true;
        } else {
            // 只要不是激活的，恢复为运行中/最小化/未启动
            if (status.pid == 0) {
                status.status = AppRunStatus::NotRunning;
            } else if (status.hwnd == nullptr) {
                status.status = AppRunStatus::Running;
            } else {
                // 可根据实际情况判断是否最小化，这里简化为Running
                if (status.status == AppRunStatus::Minimized) {
                    // 保持最小化
                } else {
                    status.status = AppRunStatus::Running;
                }
            }
        }
    }
    emit statusChanged();
} 