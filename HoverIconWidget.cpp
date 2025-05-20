#include "HoverIconWidget.h"
#include <QPainter>
#include <QDebug>
#include <QEnterEvent> // Required for QEnterEvent
#include <QMouseEvent> // Required for QMouseEvent

HoverIconWidget::HoverIconWidget(const QPixmap &icon, const QString &appName, const QString &appPath, QWidget *parent)
    : QWidget(parent), 
      m_appName(appName), 
      m_appPath(appPath),
      m_currentScaleFactor(1.0), 
      m_scaleAnimation(nullptr),
      m_iconSize(64), // Base icon size
      m_padding(8)     // Padding around icon and for text
{
    // Ensure icon is not null, provide a default if it is, or handle error
    if (icon.isNull()) {
        m_originalPixmap = QPixmap(m_iconSize, m_iconSize);
        m_originalPixmap.fill(Qt::gray); // Placeholder for null icons
        qWarning() << "HoverIconWidget: Received a null icon for" << m_appName;
    } else {
        m_originalPixmap = icon.scaled(m_iconSize, m_iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(m_originalPixmap);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_nameLabel = new QLabel(m_appName, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("color: white; background-color: transparent;"); 
    m_nameLabel->setWordWrap(true); // Allow name to wrap if too long

    QVBoxLayout *vLayout = new QVBoxLayout(); // Layout for icon and name
    vLayout->addWidget(m_iconLabel);
    vLayout->addWidget(m_nameLabel);
    vLayout->setContentsMargins(0,0,0,0); // Margins handled by overall widget padding
    vLayout->setSpacing(2); 
    
    // Main layout for the widget to enforce padding
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(vLayout);
    mainLayout->setContentsMargins(m_padding, m_padding, m_padding, m_padding);
    setLayout(mainLayout);

    // Calculate base size more accurately
    int textHeight = m_nameLabel->sizeHint().height();
    if (m_appName.isEmpty()) { // If no name, don't account for its height or spacing
        textHeight = 0;
        vLayout->setSpacing(0);
        m_nameLabel->hide();
    }
    
    m_baseSize = QSize(m_iconSize + 2 * m_padding, 
                       m_iconSize + textHeight + 2 * m_padding + (m_appName.isEmpty() ? 0 : vLayout->spacing()));
    setFixedSize(m_baseSize); 

    setupAnimation();
    setAttribute(Qt::WA_Hover); 
    setFocusPolicy(Qt::NoFocus); // Usually, these are not focusable
}

HoverIconWidget::~HoverIconWidget() = default;

void HoverIconWidget::setupAnimation() {
    m_scaleAnimation = new QPropertyAnimation(this, "scaleFactor");
    m_scaleAnimation->setDuration(120); 
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad); // Smoother curve
}

qreal HoverIconWidget::scaleFactor() const {
    return m_currentScaleFactor;
}

void HoverIconWidget::setScaleFactor(qreal factor) {
    if (qFuzzyCompare(m_currentScaleFactor, factor))
        return;

    m_currentScaleFactor = factor;
    
    int targetIconSize = static_cast<int>(m_iconSize * m_currentScaleFactor);
    
    // Scale the pixmap for the label
    if (!m_originalPixmap.isNull()) {
        m_iconLabel->setPixmap(m_originalPixmap.scaled(targetIconSize, targetIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // Recalculate the widget's fixed size based on the new icon size
    int textHeight = m_appName.isEmpty() ? 0 : m_nameLabel->fontMetrics().boundingRect(m_nameLabel->rect(), Qt::AlignCenter | Qt::TextWordWrap, m_nameLabel->text()).height();
    if (m_appName.isEmpty()) textHeight = 0; // Ensure it's 0 if no name
    
    int spacing = m_appName.isEmpty() ? 0 : static_cast<QVBoxLayout*>(layout()->itemAt(0)->layout())->spacing();

    QSize newSize(targetIconSize + 2 * m_padding,
                  targetIconSize + textHeight + 2 * m_padding + spacing);
    
    setFixedSize(newSize);
    
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
    update(); 
}

void HoverIconWidget::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    m_scaleAnimation->stop(); // Stop any ongoing animation
    m_scaleAnimation->setStartValue(m_currentScaleFactor); // Start from current scale
    m_scaleAnimation->setEndValue(1.35);      // Target scale when hovered
    m_scaleAnimation->setDirection(QAbstractAnimation::Forward); // Ensure forward direction for scaling up
    m_scaleAnimation->start();
}

void HoverIconWidget::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    m_scaleAnimation->stop(); // Stop any ongoing animation
    m_scaleAnimation->setStartValue(m_currentScaleFactor); // Start from current scale
    m_scaleAnimation->setEndValue(1.0);       // Target scale when not hovered (original size)
    // m_scaleAnimation->setDirection(QAbstractAnimation::Backward); // Direction is implicit by start/end values, but can be explicit
    m_scaleAnimation->start();
}

void HoverIconWidget::paintEvent(QPaintEvent *event) {
    // All painting is handled by the QLabels and the widget's background (if any)
    // For custom background or effects, QPainter would be used here.
    // Example: Rounded corners for the widget background (if not using stylesheets)
    // QPainter painter(this);
    // painter.setRenderHint(QPainter::Antialiasing);
    // painter.fillRect(rect(), Qt::transparent); // If WA_TranslucentBackground is set
    // QPainterPath path;
    // path.addRoundedRect(rect(), 5, 5); // 5px radius
    // painter.setClipPath(path);
    // painter.fillPath(path, QColor(100,100,100,100)); // Semi-transparent background
    
    QWidget::paintEvent(event);
}

QSize HoverIconWidget::sizeHint() const {
    int currentIconDisplaySize = static_cast<int>(m_iconSize * m_currentScaleFactor);
    int textHeight = m_appName.isEmpty() ? 0 : m_nameLabel->fontMetrics().boundingRect(m_nameLabel->rect(), Qt::AlignCenter | Qt::TextWordWrap, m_nameLabel->text()).height();
     if (m_appName.isEmpty()) textHeight = 0;
    int spacing = m_appName.isEmpty() ? 0 : static_cast<QVBoxLayout*>(layout()->itemAt(0)->layout())->spacing();
    return QSize(currentIconDisplaySize + 2 * m_padding,
                 currentIconDisplaySize + textHeight + 2 * m_padding + spacing);
}

QSize HoverIconWidget::minimumSizeHint() const {
    // Minimum could be the base size, or a fraction of it. For now, same as sizeHint.
    return sizeHint();
}

void HoverIconWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_appPath);
    }
    QWidget::mousePressEvent(event);
}

QString HoverIconWidget::applicationPath() const { return m_appPath; } 