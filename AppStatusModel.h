#pragma once
#include <QAbstractListModel>
#include <QList>
#include "AppStatus.h"

// =============================
// 应用状态模型类
// =============================
class AppStatusModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit AppStatusModel(QObject* parent = nullptr);
    // 获取行数
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    // 获取数据（用于UI显示）
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    // 获取所有支持的角色
    QHash<int, QByteArray> roleNames() const override;

    // 批量刷新所有应用状态
    void updateStatus(const QList<AppStatus>& statusList);
    // 获取单个应用状态
    AppStatus getStatus(int row) const;
    // 获取全部应用状态
    QList<AppStatus> getAllStatus() const;

    // 新增：设置某个应用为激活状态
    void setActiveApp(const QString& appPath);

signals:
    // 状态变更信号，供UI层自动刷新
    void statusChanged();

private:
    QList<AppStatus> m_statusList; // 当前所有应用状态
}; 