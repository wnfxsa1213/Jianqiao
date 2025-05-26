#include "UserView.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel> // For empty message
#include <QDebug>
#include "AppCardWidget.h" // Ensure this is included
#include <QIcon>       // For QIcon to QPixmap conversion
#include <QPixmap> // <--- 添加 QPixmap include
#include <QPushButton>
#include <QCoreApplication>
#include <QPainter> // ADDED for custom painting

UserView::UserView(QWidget *parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_dockFrame(nullptr),
      m_dockLayout(nullptr),
      m_isFirstShow(true)
{
    qDebug() << "用户视图(UserView): 已创建。";
    setupUi();
}

UserView::~UserView()
{
    qDebug() << "用户视图(UserView): 已销毁。";
    clearAppIcons(); 
}

void UserView::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    setLayout(m_mainLayout);
    m_mainLayout->setContentsMargins(0, 0, 0, 0); 

    m_dockFrame = new QFrame(this);
    m_dockFrame->setObjectName("dockFrame");
    m_dockFrame->setStyleSheet(
        "#dockFrame {"            
        "  background-color: rgba(45, 45, 45, 190);" 
        "  border-radius: 22px;"                 
        "}"
    );
    m_dockFrame->setFixedHeight(150); 
    m_dockFrame->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_dockFrame->setMinimumWidth(200); 

    m_dockLayout = new QHBoxLayout(m_dockFrame); 
    m_dockLayout->setContentsMargins(20, 10, 20, 10); 
    m_dockLayout->setSpacing(20);                     
    m_dockLayout->setAlignment(Qt::AlignCenter);      
    
    QHBoxLayout *hCenterLayout = new QHBoxLayout(); 
    hCenterLayout->addStretch(); 
    hCenterLayout->addWidget(m_dockFrame);
    hCenterLayout->addStretch(); 

    m_mainLayout->addStretch(); 
    m_mainLayout->addLayout(hCenterLayout); 
    m_mainLayout->addStretch(); 

    // Ensure main layout stretch factors are appropriate without exit button
    m_mainLayout->setStretchFactor(hCenterLayout, 0); // Let hCenterLayout take its preferred size, stretches will center it
}

void UserView::clearAppIcons() {
    if (!m_dockLayout) return;
    QLayoutItem* item;
    while ((item = m_dockLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            delete widget; 
        }
        delete item;
    }
    m_appIconWidgets.clear();
}

void UserView::setAppList(const QList<AppInfo>& apps) {
    m_cachedAppList = apps;
    qDebug() << "UserView: App list cached with" << m_cachedAppList.count() << "apps.";
    // Always populate/re-populate when the list is set.
    // If the view is invisible, it will be correctly populated when it becomes visible.
    populateAppList(m_cachedAppList); 
}

void UserView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_isFirstShow) {
        qDebug() << "UserView: First showEvent, populating list with" << m_cachedAppList.count() << "cached apps.";
        populateAppList(m_cachedAppList);
        m_isFirstShow = false;
    } else {
        qDebug() << "UserView: Subsequent showEvent.";
    }
}

// Make sure AppInfo is a known type here (should be from common_types.h)
void UserView::populateAppList(const QList<AppInfo>& apps) {
    clearAppIcons();
    qInfo() << "UserView: Populating with" << apps.count() << "apps.";

    const QSize iconSize(64, 64); // Define a standard icon size, adjust as needed

    for (const AppInfo& appInfo : apps) {
        if (appInfo.path.isEmpty()) {
            qWarning() << "UserView: Skipping app with empty path:" << appInfo.name;
            continue;
        }
        // qInfo() << "Populating UserView with app:" << appInfo.name << "Path:" << appInfo.path << "Icon null:" << appInfo.icon.isNull();
        
        QPixmap pixmap = appInfo.icon.pixmap(iconSize); // <--- 将 QIcon 转换为 QPixmap
        if (pixmap.isNull() && !appInfo.icon.isNull()) {
            // Fallback if specific size fails, try to get any pixmap
            QList<QSize> availableSizes = appInfo.icon.availableSizes();
            if (!availableSizes.isEmpty()) {
                pixmap = appInfo.icon.pixmap(availableSizes.first());
            }
        }
        if (pixmap.isNull()){
            qWarning() << "UserView: Failed to get pixmap for" << appInfo.name << "Path:" << appInfo.path << "Original icon was null:" << appInfo.icon.isNull();
            // Use a default placeholder pixmap if you have one
            // pixmap = QPixmap(":/icons/default_app_icon.png"); // Example
        }

        HoverIconWidget* iconWidget = new HoverIconWidget(pixmap, appInfo.name, appInfo.path, this); // <--- 使用 pixmap
        m_appIconWidgets.append(iconWidget);
        m_dockLayout->addWidget(iconWidget);
        connect(iconWidget, &HoverIconWidget::clicked, this, &UserView::onIconClicked);
    }

    if (m_dockLayout) { // Ensure layout exists
        m_dockLayout->activate(); // Try to enforce layout update
    }
    if (m_dockFrame && m_dockFrame->layout()) { // Also try activating frame's layout
         m_dockFrame->layout()->activate();
    }
    update(); // Repaint the UserView itself
}

void UserView::setCurrentBackground(const QString& imagePath) {
    if (imagePath.isEmpty()) {
        qDebug() << "UserView: Received empty image path, clearing background.";
        m_currentBackground = QPixmap(); // Set to a null pixmap
    } else {
        QPixmap newBg(imagePath);
        if (newBg.isNull()) {
            qWarning() << "UserView: Failed to load background image from:" << imagePath;
            // m_currentBackground = QPixmap(); // Optionally clear or keep old if load fails
            // Or set a default solid color background in paintEvent if m_currentBackground is null
        } else {
            m_currentBackground = newBg;
            qDebug() << "UserView: Background image loaded from:" << imagePath;
        }
    }
    update(); // Trigger a repaint
}

void UserView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    if (!m_currentBackground.isNull()) {
        // Draw the background image, scaled to fill the widget
        painter.drawPixmap(rect(), m_currentBackground);
    } else {
        // Fallback: If no background image or it failed to load, fill with a default color
        painter.fillRect(rect(), QColor(30, 30, 30)); // Default dark gray
    }

    // It's VERY IMPORTANT to call the base class paintEvent if you are not handling
    // all painting for child widgets yourself, OR if UserView itself has Q_OBJECT and uses stylesheets for other properties.
    // However, since m_dockFrame is a child widget, its painting is generally handled after the parent's paintEvent.
    // If m_dockFrame (or other children) are not appearing, uncommenting the line below might be necessary,
    // but it can also interfere if you've fully painted the background.
    // QWidget::paintEvent(event); 
}

void UserView::onIconClicked(const QString& appPath) {
    qDebug() << "用户视图(UserView): 图标点击，请求启动应用:" << appPath;
    emit applicationLaunchRequested(appPath);
}
