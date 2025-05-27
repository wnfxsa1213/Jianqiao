#include "AppCardWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug> // For logging
#include <QPixmap> // Added for icon display
#include <QFile>   // For loading QSS
#include <QMovie>  // For loading animation
#include <QStyle> // Added for style()->polish/unpolish
#include <QEasingCurve> // For animation easing
#include <QResizeEvent> // For resize event handling

AppCardWidget::AppCardWidget(const QString& appName, const QString& appPath, const QIcon& appIcon, QWidget *parent)
    : QWidget{parent}, 
      m_appName(appName), 
      m_appPath(appPath), 
      m_appIcon(appIcon), 
      m_isLoading(false),
      m_scaleFactor(DEFAULT_SCALE),
      m_scaleAnimation(nullptr)
{
    setupUi();
    setObjectName("appCard"); // ID for QSS

    // Load QSS from resource
    QFile styleFile(":/styles/AppCardWidget.qss"); 
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        this->setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        qWarning() << "AppCardWidget: Could not load QSS file.";
    }
    
    // 创建缩放动画
    m_scaleAnimation = new QPropertyAnimation(this, "scaleFactor");
    m_scaleAnimation->setDuration(ANIMATION_DURATION);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void AppCardWidget::setupUi()
{
    // 移除使用QVBoxLayout进行垂直居中的方法
    // 改为直接在widget中放置图标并手动控制位置
    
    setFixedSize(68, 68); // 确保AppCardWidget大小固定
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setPixmap(m_appIcon.pixmap(QSize(56, 56))); // 调整图标大小
    m_iconLabel->setFixedSize(QSize(56, 56)); // 确保图标标签大小固定
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    // 取消使用布局，改为手动设置位置
    m_layout = nullptr;

    m_nameLabel = new QLabel(m_appName, this);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setVisible(false); // 默认隐藏名称标签

    m_loadingIndicatorLabel = new QLabel(this);
    m_loadingIndicatorLabel->setObjectName("loadingIndicatorLabel");
    m_loadingIndicatorLabel->setAlignment(Qt::AlignCenter);
    m_loadingIndicatorLabel->setVisible(false); // 初始状态为隐藏

    m_loadingMovie = new QMovie(":/icons/loading_spinner.gif");
    if (m_loadingMovie->isValid()) {
        m_loadingIndicatorLabel->setMovie(m_loadingMovie);
        m_loadingMovie->setScaledSize(QSize(32, 32)); 
    } else {
        qDebug() << "AppCardWidget: Loading spinner GIF not found or invalid. Falling back to text.";
        m_loadingIndicatorLabel->setText("..."); 
    }
    
    // 初始化图标位置
    updateIconPosition();
}

void AppCardWidget::updateIconPosition()
{
    // 计算图标应该放置的位置，使其居中
    if (m_iconLabel) {
        int x = (width() - m_iconLabel->width()) / 2;
        int y = (height() - m_iconLabel->height()) / 2;
        
        // 应用缩放因子对图标进行缩放，同时保持居中
        QSize scaledSize(m_iconLabel->width() * m_scaleFactor, m_iconLabel->height() * m_scaleFactor);
        int scaledX = (width() - scaledSize.width()) / 2;
        int scaledY = (height() - scaledSize.height()) / 2;
        
        m_iconLabel->setGeometry(scaledX, scaledY, scaledSize.width(), scaledSize.height());
    }
    
    // 加载指示器也需要居中
    if (m_loadingIndicatorLabel && m_loadingIndicatorLabel->isVisible()) {
        int x = (width() - m_loadingIndicatorLabel->width()) / 2;
        int y = (height() - m_loadingIndicatorLabel->height()) / 2;
        m_loadingIndicatorLabel->move(x, y);
    }
}

void AppCardWidget::setScaleFactor(qreal factor)
{
    if (qFuzzyCompare(m_scaleFactor, factor))
        return;
        
    m_scaleFactor = factor;
    updateIconPosition();
    update();
}

void AppCardWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateIconPosition();
}

void AppCardWidget::setLoadingState(bool loading)
{
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    setProperty("loading", m_isLoading); 
    style()->unpolish(this); 
    style()->polish(this);

    if (m_isLoading) {
        m_iconLabel->setVisible(false);
        m_nameLabel->setVisible(false); 
        
        // 确保加载指示器在AppCardWidget中居中
        if (m_loadingIndicatorLabel->movie() == m_loadingMovie && m_loadingMovie && m_loadingMovie->isValid()) {
             m_loadingIndicatorLabel->setFixedSize(m_loadingMovie->currentPixmap().size());
        } else {
            m_loadingIndicatorLabel->adjustSize();
        }
        // 居中加载指示器
        int x = (this->width() - m_loadingIndicatorLabel->width()) / 2;
        int y = (this->height() - m_loadingIndicatorLabel->height()) / 2;
        m_loadingIndicatorLabel->setGeometry(x, y, m_loadingIndicatorLabel->width(), m_loadingIndicatorLabel->height());
        m_loadingIndicatorLabel->setVisible(true);

        if (m_loadingMovie && m_loadingMovie->isValid()) {
            m_loadingMovie->start();
        }
    } else {
        if (m_loadingMovie && m_loadingMovie->isValid()) {
            m_loadingMovie->stop();
        }
        m_loadingIndicatorLabel->setVisible(false);
        m_iconLabel->setVisible(true);
        m_nameLabel->setVisible(false); 
        
        // 重置缩放因子
        if (m_scaleAnimation->state() == QAbstractAnimation::Running) {
            m_scaleAnimation->stop();
        }
        m_scaleFactor = DEFAULT_SCALE;
        updateIconPosition();
    }
    update(); 
}

void AppCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_isLoading) { // 只有在非加载状态下才发射信号
            qDebug() << "AppCardWidget:" << m_appName << "clicked.";
            setLoadingState(true); // 立即显示加载状态
            emit launchAppRequested(m_appPath, m_appName);
        }
    }
    QWidget::mousePressEvent(event);
}

void AppCardWidget::enterEvent(QEnterEvent *event)
{
    // 鼠标进入时启动放大动画
    if (!m_isLoading && m_scaleAnimation) {
        m_scaleAnimation->stop();
        m_scaleAnimation->setStartValue(m_scaleFactor);
        m_scaleAnimation->setEndValue(HOVER_SCALE);
        m_scaleAnimation->start();
    }
    QWidget::enterEvent(event);
}

void AppCardWidget::leaveEvent(QEvent *event)
{
    // 鼠标离开时启动缩小动画
    if (!m_isLoading && m_scaleAnimation) {
        m_scaleAnimation->stop();
        m_scaleAnimation->setStartValue(m_scaleFactor);
        m_scaleAnimation->setEndValue(DEFAULT_SCALE);
        m_scaleAnimation->start();
    }
    QWidget::leaveEvent(event);
}