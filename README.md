# 剑鞘系统 JianqiaoSystem

## 项目简介

"剑鞘系统"是一款基于 C++/Qt6 的 Windows 平台全屏受控环境系统。它通过全屏遮挡、快捷键拦截、白名单应用管理等手段，为考试、展厅、信息终端等场景提供安全、沉浸式的操作环境。系统支持管理员热键唤醒、白名单应用配置、用户模式下系统快捷键拦截等功能，UI 现代美观，支持自定义。

---

## 主要特性

- **全屏霸屏**：主窗口全屏覆盖桌面和任务栏，防止用户访问系统桌面。
- **管理员模式**：通过自定义组合键唤醒，输入密码后进入管理界面，支持白名单应用管理、密码修改、热键配置等。
- **用户模式**：仅能启动和切换白名单应用，所有非白名单应用和大部分系统快捷键被拦截。
- **快捷键拦截**：支持拦截 Win、Alt+Tab、Win+D、Ctrl+Shift+Esc、Alt+F4 等常见系统快捷键，提升安全性。
- **白名单应用管理**：管理员可通过 UI 添加、删除、编辑允许用户启动的应用。
- **配置文件自定义**：所有核心配置（密码、热键、白名单、拦截快捷键等）均存储于 `config.json`，便于备份和迁移。
- **现代化 UI/UX**：界面风格简洁、动画流畅，支持自定义背景和主题色。

---

## 系统架构

- **JianqiaoCoreShell**：主程序入口和主窗口，负责模式切换、UI管理、模块加载。
- **AdminModule**：管理员登录、白名单管理、密码和热键配置。
- **UserModeModule**：用户模式主界面，白名单应用启动与监控。
- **SystemInteractionModule**：底层系统交互，键盘钩子、窗口层级、快捷键拦截等。
- **UserView/AdminDashboardView**：分别为用户和管理员的主界面。

详细架构和技术细节见 `doc/开发文档.txt`。

---

## 编译与运行

### 环境要求

- Windows 10/11
- Qt 6.5 及以上（推荐 6.9.0）
- MSVC 2022
- CMake 3.16 及以上
- 推荐使用 Qt Creator 或 Visual Studio（需安装 Qt VS Tools）

### 编译步骤

1. 安装 Qt 6.x（确保 Qt Core/Gui/Widgets/Concurrent/GuiPrivate 等模块）。
2. 克隆本项目到本地。
3. 使用 Qt Creator 打开 `CMakeLists.txt`，选择合适的 Kit（如 Desktop Qt 6.9.0 MSVC2022 64bit）。
4. 构建项目（Ctrl+B）。
5. 运行生成的 `JianqiaoSystem.exe`。

> **注意**：首次运行会在 `C:/Users/你的用户名/AppData/Local/JianqiaoSystem/config.json` 自动生成配置文件。

---

## 配置文件说明（`config.json`）

配置文件路径：  
`C:/Users/你的用户名/AppData/Local/JianqiaoSystem/config.json`

### 主要字段

```json
{
  "admin_password": "sha256哈希",
  "shortcuts": {
    "admin_login": {
      "key_sequence": ["VK_SHIFT", "VK_CONTROL", "VK_MENU", "A"]
    }
  },
  "whitelist_apps": [
    {
      "name": "WPS",
      "path": "C:/Users/xxx/AppData/Local/Kingsoft/WPS Office/ksolaunch.exe",
      "windowFindingHints": {
        "allowNonTopLevel": true,
        "minScore": 50,
        "primaryClassName": "OpusApp",
        "titleContains": "WPS Office"
      }
    }
  ],
  "user_mode_settings": {
    "blocked_keys": ["VK_LWIN", "VK_RWIN"],
    "blocked_key_combinations": [
      ["VK_MENU", "VK_TAB"],
      ["VK_CONTROL", "VK_SHIFT", "VK_ESCAPE"],
      ["VK_LWIN", "D"],
      ...
    ]
  }
}
```

#### 说明

- `admin_password`：管理员密码的 SHA256 哈希，初始为 `123456`。
- `shortcuts.admin_login.key_sequence`：管理员登录热键（如 Ctrl+Shift+Alt+A）。
- `whitelist_apps`：白名单应用列表，`name`为显示名，`path`为可执行文件路径。
- `user_mode_settings.blocked_keys`：用户模式下需拦截的单键（如 Win 键）。
- `user_mode_settings.blocked_key_combinations`：需拦截的组合键（如 Alt+Tab、Win+D 等）。

> **修改配置后建议重启程序生效。**

---

## 目录结构简述

- `main.cpp`：程序入口
- `JianqiaoCoreShell.*`：主窗口与核心调度
- `AdminModule.*`、`AdminDashboardView.*`、`AdminLoginView.*`：管理员相关
- `UserModeModule.*`、`UserView.*`：用户模式相关
- `SystemInteractionModule.*`：系统底层交互
- `config.json`：主配置文件
- `doc/`：开发文档、评估报告、补充说明
- `images/`、`icons/`、`styles/`：资源文件

---

## 开发与维护建议

- 详细开发流程、技术难点、UI/UX 设计建议见 `doc/开发文档.txt`、`doc/文档补充.txt`。
- 代码注释和模块划分清晰，便于二次开发和维护。
- 建议持续完善快捷键拦截、窗口层级、UI 细节和错误日志。

---

## 致谢

本项目由"雪鸮团队"开发，感谢所有贡献者和测试者的支持！

如需进一步技术支持或定制开发，请联系项目维护者。 