#pragma once
#include <QString>
#include <QIcon>
#include <QDateTime>
#include <windows.h>

// =============================
// 应用运行状态枚举
// =============================
enum class AppRunStatus {
    NotRunning,   // 未启动
    Running,      // 运行中
    Minimized,    // 最小化
    Activated,    // 当前激活
    Error         // 异常
};

// =============================
// 单个应用的状态信息结构体
// =============================
struct AppStatus {
    QString appName;      // 应用名称
    QString exePath;      // 可执行文件路径
    QIcon icon;           // 应用图标
    AppRunStatus status;  // 当前运行状态
    DWORD pid;            // 进程ID（0表示未运行）
    HWND hwnd;            // 窗口句柄（nullptr表示无窗口）
    QDateTime lastActive; // 最后活跃时间
}; 