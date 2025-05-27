#include "UserView.h"
#include "AppCardWidget.h" // Ensure this is included
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>       // For empty message (if needed)
#include <QDebug>
#include <QPainter>     // For custom painting
#include <QTimer>       // For launch timers
#include <QResizeEvent> // For resizeEvent

UserView::UserView(QWidget *parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_dockFrame(nullptr),
      m_dockScrollArea(nullptr),
      m_dockScrollContentWidget(nullptr),
      m_dockItemsLayout(nullptr),
      m_isFirstShow(true)
{
    qDebug() << "用户视图(UserView): 已创建。";
    setupUi();
    this->setObjectName("userView"); 
    // Ensure background is transparent if not painting image, so QSS on parent or paint event can control it.
    // setAttribute(Qt::WA_StyledBackground, false); // Let paintEvent handle background
}

UserView::~UserView()
{
    qDebug() << "用户视图(UserView): 已销毁。";
    clearAppCards(); 
}

void UserView::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_mainLayout->addStretch(1); // Pushes the dock to the bottom

    // Centering layout for the dock panel
    QHBoxLayout* centeringLayout = new QHBoxLayout();
    centeringLayout->setContentsMargins(0,0,0,0); // Control margins via QSS on dockPanel or here
    centeringLayout->setSpacing(0);
    centeringLayout->addStretch(1);

    m_dockFrame = new QFrame(this); 
    m_dockFrame->setObjectName("dockPanel");
    m_dockFrame->setFixedHeight(90); 
    // Width will be content-driven up to a point, or can be constrained by UserView's width
    m_dockFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // margins for dockPanel relative to UserView edges are set in QSS
    // internal padding for scrollArea from dockFrame edges are set in QSS for dockPanel

    QVBoxLayout* dockFrameInternalLayout = new QVBoxLayout(m_dockFrame); // Layout for m_dockScrollArea within m_dockFrame
    dockFrameInternalLayout->setContentsMargins(0, 5, 0, 5); // Top/Bottom padding inside dock frame for scroll area
    
    m_dockScrollArea = new QScrollArea(m_dockFrame); 
    m_dockScrollArea->setObjectName("dockScrollArea");
    m_dockScrollArea->setWidgetResizable(true);
    m_dockScrollArea->setFrameShape(QFrame::NoFrame);
    m_dockScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dockScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_dockScrollArea->setFixedHeight(70); // Height for the scrollable content area

    m_dockScrollContentWidget = new QWidget(); 
    m_dockScrollContentWidget->setObjectName("dockScrollContentWidget");
    m_dockScrollArea->setWidget(m_dockScrollContentWidget);

    m_dockItemsLayout = new QHBoxLayout(m_dockScrollContentWidget);
    m_dockItemsLayout->setContentsMargins(10, 0, 10, 0); // Left/Right padding for the first/last card from scroll area edge
    m_dockItemsLayout->setSpacing(10); 
    m_dockItemsLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // m_dockItemsLayout->addStretch(); // So cards align left, and don't spread if few

    dockFrameInternalLayout->addWidget(m_dockScrollArea);
    m_dockFrame->setLayout(dockFrameInternalLayout);

    centeringLayout->addWidget(m_dockFrame);
    centeringLayout->addStretch(1);
    m_mainLayout->addLayout(centeringLayout);

    setLayout(m_mainLayout);

    // Apply QSS for the dock panel and scroll area
    // It's better to load this from a UserView.qss file if it gets complex
    QString userViewStyleSheet = QString(
        "QFrame#dockPanel {"
        "    background-color: rgba(45, 45, 45, 0.88);"
        "    border-radius: 20px;"
        "    margin-left: 20px;"
        "    margin-right: 20px;"
        "    margin-bottom: 10px;"
        "    padding: 0px 10px;" /* Horizontal padding inside dock for scroll area */
        "}"
        "QScrollArea#dockScrollArea {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "QWidget#dockScrollContentWidget {"
        "    background-color: transparent;"
        "}"
        "QScrollBar:horizontal {"
        "    height: 8px;"
        "    background: rgba(0,0,0,0.1);"
        "    margin: 0px 0px 2px 0px;" /* margin from bottom of scroll area */
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: rgba(255,255,255,0.35);"
        "    min-width: 25px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    background: none; border: none; width: 0px;"
        "}"
    );
    this->setStyleSheet(userViewStyleSheet);
}

void UserView::clearAppCards() {
    for (AppCardWidget* card : qAsConst(m_appCards)) {
        if (card) {
            m_dockItemsLayout->removeWidget(card);
            card->deleteLater();
        }
    }
    m_appCards.clear();

    // Also clear any remaining launch timers, just in case
    qDeleteAll(m_launchTimers);
    m_launchTimers.clear();
    m_launchingApps.clear();
}

void UserView::setAppList(const QList<AppInfo>& apps) {
    m_currentApps = apps;
    if (isVisible() || m_isFirstShow) { // Populate immediately if view is visible or first show
        populateAppList(apps);
    }
    // If not visible and not first show, populateAppList will be called in showEvent
}

void UserView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_isFirstShow) {
        populateAppList(m_currentApps); // Populate on first show
        m_isFirstShow = false;
    }
}

void UserView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event); // Call base class
    // If m_dockFrame width needs to be dynamically adjusted based on UserView width 
    // (e.g., to not exceed UserView width), calculations could go here.
    // For now, it's mostly Preferred size up to UserView boundaries due to centeringLayout.
    // The QSS margins also help keep it from edges.
    // repaint(); // May be needed if background calculations depend on size
}

void UserView::populateAppList(const QList<AppInfo>& apps) {
    clearAppCards();
    m_currentApps = apps;
    
    if (!m_dockItemsLayout) {
        qWarning() << "UserView::populateAppList: m_dockItemsLayout is null!";
        return;
    }

    for (const AppInfo& appInfo : qAsConst(m_currentApps)) {
        QIcon icon = appInfo.icon;
        if (icon.isNull()) {
             qWarning() << "UserView: Icon for" << appInfo.name << "is null. AppCardWidget might use its default.";
        }

        AppCardWidget *card = new AppCardWidget(appInfo.name, appInfo.path, icon, m_dockScrollContentWidget);
        connect(card, &AppCardWidget::launchAppRequested, this, &UserView::onCardLaunchRequested);
        
        m_dockItemsLayout->addWidget(card);
        m_appCards.append(card);
    }
    
    if (m_dockScrollContentWidget) { 
        m_dockScrollContentWidget->adjustSize();
    }
}

void UserView::setCurrentBackground(const QString& imagePath) {
    if (imagePath.isEmpty()) {
        m_currentBackground = QPixmap();
        qDebug() << "UserView: Background image cleared.";
    } else {
        if (!m_currentBackground.load(imagePath)) {
            qWarning() << "UserView: Failed to load background image from" << imagePath;
            m_currentBackground = QPixmap(); // Clear on failure
        }
         qDebug() << "UserView: Background image set to" << imagePath;
    }
    update(); // Request a repaint to show the new background
}

void UserView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    if (!m_currentBackground.isNull()) {
        painter.drawPixmap(this->rect(), m_currentBackground);
    } else {
        painter.fillRect(this->rect(), QColor(30, 30, 30)); // Default dark background
    }
}

AppCardWidget* UserView::findAppCardByPath(const QString& appPath) const {
    for (AppCardWidget* card : qAsConst(m_appCards)) {
        if (card && card->getAppPath() == appPath) {
            return card;
        }
    }
    return nullptr;
}

void UserView::onCardLaunchRequested(const QString& appPath, const QString& appName) {
    qDebug() << "UserView: Launch requested for" << appName << "at" << appPath;
    emit applicationLaunchRequested(appPath, appName);
}

void UserView::setAppLoadingState(const QString& appPath, bool isLoading) {
    AppCardWidget* card = findAppCardByPath(appPath);
    if (card) {
        card->setLoadingState(isLoading);
    } else {
        qWarning() << "UserView: Could not find app card for path:" << appPath << "to set loading state.";
    }

    if (isLoading) {
        if (!m_launchingApps.contains(appPath)) {
            m_launchingApps.insert(appPath);
            QTimer* timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this, [this, appPath]() { this->onLaunchTimerTimeout(appPath); });
            m_launchTimers.insert(appPath, timer);
            timer->start(LAUNCH_TIMEOUT_MS);
        }
    } else {
        m_launchingApps.remove(appPath);
        if (m_launchTimers.contains(appPath)) {
            QTimer* timer = m_launchTimers.value(appPath);
            if (timer) {
                timer->stop();
                timer->deleteLater();
            }
            m_launchTimers.remove(appPath);
        }
    }
}

void UserView::onLaunchTimerTimeout(const QString& appPath) {
    if (m_launchingApps.contains(appPath)) {
        qWarning() << "UserView: Launch timed out for" << appPath;
        this->setAppLoadingState(appPath, false);
    } else {
        qWarning() << "UserView: Launch timer for" << appPath << "fired, but app not in launching state or path mismatch.";
    }
}
