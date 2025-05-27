#include "AppCardWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug> // For logging
#include <QPixmap> // Added for icon display
#include <QFile>   // For loading QSS
#include <QMovie>  // For loading animation
#include <QStyle> // Added for style()->polish/unpolish

AppCardWidget::AppCardWidget(const QString& appName, const QString& appPath, const QIcon& appIcon, QWidget *parent)
    : QWidget{parent}, m_appName(appName), m_appPath(appPath), m_appIcon(appIcon), m_isLoading(false)
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
}

void AppCardWidget::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0); // QSS controls padding
    m_layout->setSpacing(2); // Minimal spacing if name were visible

    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setPixmap(m_appIcon.pixmap(QSize(56, 56))); // Adjusted icon size
    m_iconLabel->setFixedSize(QSize(56, 56)); // Ensure icon label takes this size
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_nameLabel = new QLabel(m_appName, this);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setVisible(false); // Hide name label by default for Dock style

    // Loading Indicator (Prefer QMovie if available, else simple text)
    m_loadingIndicatorLabel = new QLabel(this); // Will hold either text or QMovie
    m_loadingIndicatorLabel->setObjectName("loadingIndicatorLabel");
    m_loadingIndicatorLabel->setAlignment(Qt::AlignCenter);
    m_loadingIndicatorLabel->setVisible(false); // Initially hidden

    // Attempt to load a GIF for loading animation
    m_loadingMovie = new QMovie(":/icons/loading_spinner.gif"); // Path to your loading GIF in resources
    if (m_loadingMovie->isValid()) {
        m_loadingIndicatorLabel->setMovie(m_loadingMovie);
        m_loadingMovie->setScaledSize(QSize(32, 32)); // Adjust size of spinner
    } else {
        qDebug() << "AppCardWidget: Loading spinner GIF not found or invalid. Falling back to text.";
        m_loadingIndicatorLabel->setText("..."); // Fallback text
        // delete m_loadingMovie; // Optional: delete if not used to save a tiny bit of memory
        // m_loadingMovie = nullptr;
    }

    // Layout: Icon and Name label are usually visible OR loading indicator is visible
    // We'll use a QStackedLayout if complex, or just manage visibility if simple.
    // For now, we'll manage visibility and ensure they are centered.
    // QVBoxLayout will stack them, but only one (icon or loading) should be prominent.

    m_layout->addStretch(); // Push content to center if VBox is larger
    m_layout->addWidget(m_iconLabel, 0, Qt::AlignCenter);         // Icon centered
    m_layout->addWidget(m_nameLabel, 0, Qt::AlignCenter);         // Name centered (but hidden)
    m_layout->addWidget(m_loadingIndicatorLabel, 0, Qt::AlignCenter); // Loading centered
    m_layout->addStretch(); // Push content to center

    setLayout(m_layout);
    // Size constraints are now primarily from QSS (min/max width/height for appCard object)
    // setFixedSize can be used if QSS is not sufficient, but QSS is preferred.
}

void AppCardWidget::setLoadingState(bool loading)
{
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    setProperty("loading", m_isLoading); // For QSS styling [loading="true"]
    style()->unpolish(this); // Re-apply QSS
    style()->polish(this);

    if (m_isLoading) {
        m_iconLabel->setVisible(false);
        m_nameLabel->setVisible(false); // Ensure name is also hidden
        m_loadingIndicatorLabel->setVisible(true);
        if (m_loadingMovie && m_loadingMovie->isValid()) {
            m_loadingMovie->start();
        } else {
            // Text is already set in setupUi or ensure it's visible
            m_loadingIndicatorLabel->setText("..."); 
        }
    } else {
        if (m_loadingMovie && m_loadingMovie->isValid()) {
            m_loadingMovie->stop();
        }
        m_loadingIndicatorLabel->setVisible(false);
        m_iconLabel->setVisible(true);
        m_nameLabel->setVisible(false); // Keep name hidden for Dock style
    }
    update(); // Request a repaint
}

void AppCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_isLoading) { // Only emit if not already loading
            qDebug() << "AppCardWidget:" << m_appName << "clicked.";
            setLoadingState(true); // Visually indicate loading immediately
            emit launchAppRequested(m_appPath, m_appName);
        }
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