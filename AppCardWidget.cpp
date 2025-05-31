#include "AppCardWidget.h"
#include <QWidget>
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
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QImage>
#include <QIcon>
#include <QSize>
#include <Qt>

// 辅助函数：裁剪透明边距
static QRect tightBoundingRect(const QImage& img) {
    int left = img.width(), right = 0, top = img.height(), bottom = 0;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) > 10) {
                if (x < left) left = x;
                if (x > right) right = x;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
    }
    if (left > right || top > bottom) return QRect(0,0,1,1);
    return QRect(left, top, right-left+1, bottom-top+1);
}

// 工具函数：缩放并居中pixmap（仅裁剪和缩放，不画圆底）
static QPixmap scaledCenteredPixmap(const QIcon& icon, const QSize& targetSize) {
    QPixmap origPixmap = icon.pixmap(QSize(128,128));
    if (origPixmap.isNull()) return QPixmap(targetSize); // 返回空白pixmap
    QImage img = origPixmap.toImage();
    QRect contentRect = tightBoundingRect(img);
    QImage trimmed = img.copy(contentRect);
    QPixmap scaledPixmap = QPixmap::fromImage(trimmed).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap result(targetSize);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    int x = (result.width() - scaledPixmap.width())/2;
    int y = (result.height() - scaledPixmap.height())/2;
    painter.drawPixmap(x, y, scaledPixmap);
    return result;
}

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
    // 图标健壮性处理：如icon无效则显示默认图标
    if (m_appIcon.isNull()) {
        QPixmap defaultPixmap(56, 56);
        defaultPixmap.fill(Qt::gray);
        m_appIcon = QIcon(defaultPixmap);
        qWarning() << "AppCardWidget: appIcon is null, using default icon.";
    }
    // 创建缩放动画
    m_scaleAnimation = new QPropertyAnimation(this, "scaleFactor");
    m_scaleAnimation->setDuration(ANIMATION_DURATION);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

    qreal dpi = this->logicalDpiX();
    int baseSize = 128 * (dpi / 96.0); // 96为标准DPI
    setFixedSize(baseSize, baseSize);
}

void AppCardWidget::setupUi()
{
    // 移除使用QVBoxLayout进行垂直居中的方法
    // 改为直接在widget中放置图标并手动控制位置
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setPixmap(scaledCenteredPixmap(m_appIcon, QSize(56, 56))); // 放大图标
    m_iconLabel->setFixedSize(QSize(56, 56));//设置固定大小
    m_iconLabel->setAlignment(Qt::AlignCenter);//设置对齐方式
    
    // 取消使用布局，改为手动设置位置
    m_layout = nullptr;

    m_nameLabel = new QLabel(m_appName, this);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);//
    m_nameLabel->setVisible(false); // 默认隐藏名称标签

    m_loadingIndicatorLabel = new QLabel(this);
    m_loadingIndicatorLabel->setObjectName("loadingIndicatorLabel");
    m_loadingIndicatorLabel->setAlignment(Qt::AlignCenter);
    m_loadingIndicatorLabel->setVisible(false); // 初始状态为隐藏

    m_loadingMovie = new QMovie(":/icons/loading_spinner.gif");
    if (m_loadingMovie->isValid()) {
        m_loadingIndicatorLabel->setMovie(m_loadingMovie);
        m_loadingMovie->setScaledSize(QSize(32, 32)); //设置缩放大小
    } else {
        qDebug() << "AppCardWidget: Loading spinner GIF not found or invalid. Falling back to text.";
        m_loadingIndicatorLabel->setText("..."); 
    }
    
    // 初始化图标位置
    updateIconPosition();
}

void AppCardWidget::updateIconPosition()
{
    if (m_iconLabel) {
        // 计算缩放后图标的目标尺寸，最大为卡片本身尺寸的95%
        int iconBaseSize = qMin(width(), height()) * 0.70; // 图标更大
        QSize scaledSize = QSize(iconBaseSize * m_scaleFactor, iconBaseSize * m_scaleFactor);

        // 生成缩放后的pixmap并设置到label
        m_iconLabel->setPixmap(scaledCenteredPixmap(m_appIcon, scaledSize));
        m_iconLabel->setFixedSize(scaledSize);
        m_iconLabel->setAttribute(Qt::WA_TranslucentBackground, true); // 保证背景透明

        // 让图标始终居中于卡片
        int scaledX = (width() - scaledSize.width()) / 2;
        int scaledY = (height() - scaledSize.height()) / 2 - 6;
        m_iconLabel->move(scaledX, scaledY);
    }
    // 加载指示器同理，始终居中
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