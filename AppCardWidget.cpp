#include "AppCardWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug> // For logging
#include <QPixmap> // Added for icon display

AppCardWidget::AppCardWidget(const QString& appName, const QString& appPath, const QIcon& appIcon, QWidget *parent)
    : QWidget{parent}, m_appName(appName), m_appPath(appPath), m_appIcon(appIcon)
{
    setupUi();
    setFixedSize(120, 120); // Adjusted size for icon + text
    setObjectName("appCard");
    setStyleSheet(
        "#appCard {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #555555, stop:1 #333333);"
        "  border: 1px solid #2a2a2a;"
        "  border-radius: 10px;"
        "  padding: 8px;"
        "}"
        "#appCard:hover {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #666666, stop:1 #444444);"
        "  border: 1px solid #55aaff;"
        "}"
        "#appCard QLabel#iconLabel {" // Style for icon label
        "  background-color: transparent;"
        "  padding-bottom: 5px;" // Space between icon and name
        "}"
        "#appCard QLabel#nameLabel {" // Style for name label
        "  color: #eeeeee;"
        "  font-weight: normal;"
        "  font-size: 10pt;"
        "  background-color: transparent;"
        "}"
    );
}

void AppCardWidget::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(3); // Reduced spacing

    m_iconLabel = new QLabel();
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setAlignment(Qt::AlignCenter);
    if (!m_appIcon.isNull()) {
        m_iconLabel->setPixmap(m_appIcon.pixmap(QSize(64, 64))); // Display icon
    } else {
        m_iconLabel->setText("[无图标]"); // Fallback text if icon is null
        m_iconLabel->setFixedSize(64,64);
        m_iconLabel->setStyleSheet("color: #aaaaaa; font-style: italic;");
    }
    m_layout->addWidget(m_iconLabel, 0, Qt::AlignCenter); // Stretch factor 1 for icon

    m_nameLabel = new QLabel(m_appName);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    m_layout->addWidget(m_nameLabel, 0, Qt::AlignCenter); // Stretch factor 0 for name

    setLayout(m_layout);
}

void AppCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << "AppCardWidget: '" << m_appName << "' clicked. Path:" << m_appPath;
        emit launchAppRequested(m_appPath, m_appName);
    }
    QWidget::mousePressEvent(event);
}

void AppCardWidget::enterEvent(QEnterEvent *event)
{
    // Could enhance with property animation for smoother hover effect
    // For now, QSS :hover handles it.
    QWidget::enterEvent(event);
}

void AppCardWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
} 