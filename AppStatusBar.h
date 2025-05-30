#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include "AppStatusModel.h"

// =============================
// 应用状态栏控件
// =============================
class AppStatusBar : public QWidget {
    Q_OBJECT
public:
    explicit AppStatusBar(QWidget* parent = nullptr);
    // 绑定数据模型
    void setModel(AppStatusModel* model);

signals:
    // 用户点击某个应用卡片，传递索引
    void appCardClicked(int index);
    // 用户右键某个应用卡片，传递索引和全局坐标
    void appCardRightClicked(int index, const QPoint& globalPos);

protected:
    // 监听模型变化，刷新UI
    void refreshCards();

private:
    AppStatusModel* m_model;           // 数据模型
    QHBoxLayout* m_layout;             // 横向布局
    QList<QWidget*> m_cardWidgets;     // 当前所有卡片控件
}; 