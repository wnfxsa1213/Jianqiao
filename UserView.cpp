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
    centeringLayout->setContentsMargins(0,0,0,0); 
    centeringLayout->setSpacing(0);
    // centeringLayout->addStretch(1); // Left stretch - REMOVE or set to 0 if m_dockFrame gets a >0 stretchFactor

    m_dockFrame = new QFrame(this); 
    m_dockFrame->setObjectName("dockPanel");
    m_dockFrame->setFixedHeight(90); 
    m_dockFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Keep Expanding horizontally

    QVBoxLayout* dockFrameInternalLayout = new QVBoxLayout(m_dockFrame); 
    dockFrameInternalLayout->setContentsMargins(0, 5, 0, 5);
    
    m_dockScrollArea = new QScrollArea(m_dockFrame); 
    m_dockScrollArea->setObjectName("dockScrollArea");
    m_dockScrollArea->setWidgetResizable(true);
    m_dockScrollArea->setFrameShape(QFrame::NoFrame);
    m_dockScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dockScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_dockScrollArea->setFixedHeight(70); 
    m_dockScrollArea->setAlignment(Qt::AlignCenter);

    m_dockScrollContentWidget = new QWidget(); 
    m_dockScrollContentWidget->setObjectName("dockScrollContentWidget");
    m_dockScrollArea->setWidget(m_dockScrollContentWidget);

    m_dockItemsLayout = new QHBoxLayout(m_dockScrollContentWidget);
    m_dockItemsLayout->setContentsMargins(5, 0, 5, 0);
    m_dockItemsLayout->setSpacing(5);
    m_dockItemsLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    dockFrameInternalLayout->addWidget(m_dockScrollArea);
    m_dockFrame->setLayout(dockFrameInternalLayout);

    // Adjust stretch factors for centeringLayout
    centeringLayout->addStretch(0); // Minimal stretch on the left
    centeringLayout->addWidget(m_dockFrame, 1); // Give m_dockFrame higher priority for space
    centeringLayout->addStretch(0); // Minimal stretch on the right
    m_mainLayout->addLayout(centeringLayout);

    setLayout(m_mainLayout);

    QString userViewStyleSheet = QString(
        "QFrame#dockPanel {"\
        "    background-color: rgba(220, 220, 220, 0.85);"
        "    border-radius: 18px;"
        "    margin-left: 50px;"
        "    margin-right: 50px;"
        "    margin-bottom: 15px;"
        "    padding: 2px 8px 8px 8px;"
        "}"\
        "QScrollArea#dockScrollArea {"\
        "    background-color: transparent;"\
        "    border: none;"\
        "}"\
        "QWidget#dockScrollContentWidget {"\
        "    background-color: transparent;"\
        "}"\
        "QScrollBar:horizontal {"\
        "    height: 6px;"
        "    background: rgba(0,0,0,0.1);"
        "    margin: 0px 0px 0px 0px;"
        "    border-radius: 3px;"\
        "}"\
        "QScrollBar::handle:horizontal {"\
        "    background: rgba(0,0,0,0.3);"
        "    min-width: 20px;"\
        "    border-radius: 3px;"\
        "}"\
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"\
        "    background: none; border: none; width: 0px;"\
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
    // Populate immediately if view is visible or first show
    // if (isVisible() || m_isFirstShow) { 
    //    populateAppList(apps);
    // }
    // If not visible and not first show, populateAppList will be called in showEvent

    // Always populate the list when it's set, regardless of visibility or first show status.
    // The showEvent logic for m_isFirstShow can remain as a fallback for initial population 
    // if setAppList somehow isn't called before the first show, though it should be.
    populateAppList(apps); 
}

void UserView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_isFirstShow) {
        populateAppList(m_currentApps); // Populate on first show
        m_isFirstShow = false;
    }
}

void UserView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateDockFrameOptimalWidth();
    // If m_dockFrame width needs to be dynamically adjusted based on UserView width 
    // (e.g., to not exceed UserView width), calculations could go here.
    // For now, it's mostly Preferred size up to UserView boundaries due to centeringLayout.
    // The QSS margins also help keep it from edges.
    // repaint(); // May be needed if background calculations depend on size
}

void UserView::populateAppList(const QList<AppInfo>& apps) {
    qDebug() << "UserView::populateAppList - Received" << apps.count() << "apps. Clearing existing cards.";
    for (const AppInfo& app : apps) {
        qDebug() << "  App:" << app.name << "Path:" << app.path << "Icon isNull:" << app.icon.isNull();
    }
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
    updateDockFrameOptimalWidth();
}

void UserView::updateDockFrameOptimalWidth() {
    if (!m_dockFrame || !m_dockScrollContentWidget || !this->parentWidget()) {
        // Not fully setup or no parent to get width from, bail out
        return;
    }

    // The width of m_dockFrame is now primarily controlled by its SizePolicy (Expanding)
    // and its QSS margins (e.g., margin-left: 50px; margin-right: 50px;)
    // when placed within the centeringLayout (QHBoxLayout with stretches).
    // We should avoid overriding this with setFixedWidth() if we want it to be responsive
    // to UserView's width while respecting those margins.

    // The main purpose of this function, if not setting a fixed width, 
    // would be to ensure the scrollable content is correctly sized.
    // m_dockScrollContentWidget->adjustSize() is called in populateAppList after items are added.
    // Calling it here again (e.g. on UserView resize) ensures that if the scroll area's
    // own size changes, the content widget can re-evaluate its ideal size if necessary,
    // though with m_dockScrollArea->setWidgetResizable(true), this might be handled.

    // For now, let's assume m_dockScrollContentWidget->adjustSize() in populateAppList is sufficient
    // for content width changes. If UserView resizes, m_dockFrame resizes due to layout,
    // and QScrollArea should handle its viewport correctly.

    // If specific logic IS needed to adjust m_dockFrame's width dynamically (e.g. based on content
    // up to a certain max, then allow scrolling), the original complex logic would be needed.
    // But based on "就达到这种长度和宽度就好" for the provided screenshot, a responsive
    // width respecting QSS margins seems appropriate. 

    // Therefore, we remove explicit setFixedWidth() calls from here to let the layout manage it.
    // If m_dockScrollContentWidget's size needs re-evaluation upon UserView resize for some reason:
    // m_dockScrollContentWidget->adjustSize(); 
    // However, this is usually for when the content *itself* changes, not just the container.
    // Given m_dockScrollArea->setWidgetResizable(true), the scroll area should adapt.
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
