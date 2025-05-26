#include "AdminLoginView.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>

AdminLoginView::AdminLoginView(QWidget *parent)
    : QWidget{parent}
    , m_infoLabel(new QLabel(this))
{
    setupUi();
    qDebug() << "管理员登录视图(AdminLoginView): 已创建。";
}

AdminLoginView::~AdminLoginView()
{
    qDebug() << "管理员登录视图(AdminLoginView): 已销毁。";
}

void AdminLoginView::setupUi()
{
    setWindowTitle("管理员登录");
    setObjectName("AdminLoginView");
    setFixedSize(300, 220);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    QFile styleFile(QCoreApplication::applicationDirPath() + "/AdminLoginView.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        this->setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "管理员登录视图(AdminLoginView): 成功加载 AdminLoginView.qss";
    } else {
        qWarning() << "管理员登录视图(AdminLoginView): 无法加载 AdminLoginView.qss: " << styleFile.errorString();
        this->setStyleSheet("background-color: #f0f0f0; border: 1px solid #cccccc; border-radius: 5px;");
    }

    m_infoLabel->setText("请输入管理员密码");
    m_infoLabel->setAlignment(Qt::AlignCenter);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_loginButton = new QPushButton("登录", this);
    m_loginButton->setObjectName("m_loginButton");
    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "管理员登录视图(AdminLoginView): '登录'按钮被点击。";
        emit loginAttempt(m_passwordEdit->text());
    });

    m_exitButton = new QPushButton("取消/退出登录", this);
    m_exitButton->setObjectName("m_exitButton");
    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "管理员登录视图(AdminLoginView): '退出登录'按钮被点击。";
        emit userRequestsExit();
    });

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_passwordEdit);
    mainLayout->addWidget(m_loginButton);
    mainLayout->addStretch();
    mainLayout->addWidget(m_exitButton);
    setLayout(mainLayout);
}

#include <QScreen>
#include <QGuiApplication>
void AdminLoginView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        move((screenGeometry.width() - width()) / 2, (screenGeometry.height() - height()) / 2);
    }
    qDebug() << "管理员登录视图(AdminLoginView): 显示事件触发，尝试居中。";
}

void AdminLoginView::hideEvent(QHideEvent *event)
{
    qDebug() << "管理员登录视图(AdminLoginView): hideEvent triggered.";
    emit viewHidden(); // Emit the signal when the view is hidden
    QWidget::hideEvent(event);
}

void AdminLoginView::notifyLoginResult(bool success)
{
    if (success) {
        m_infoLabel->setText("登录成功！");
        m_passwordEdit->clear();
        m_passwordEdit->setEnabled(false);
        m_loginButton->setEnabled(false);
        qDebug() << "管理员登录视图(AdminLoginView): 登录成功，UI已更新。";
    } else {
        m_infoLabel->setText("密码错误，请重试！");
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
        qDebug() << "管理员登录视图(AdminLoginView): 登录失败，UI已更新。";
    }
    if (m_loginButton) m_loginButton->setEnabled(true);
    qDebug() << "管理员登录视图(AdminLoginView): UI已重置为初始状态。";
}

void AdminLoginView::resetUI()
{
    if (m_infoLabel) m_infoLabel->setText("请输入管理员密码");
    if (m_passwordEdit) {
        m_passwordEdit->clear();
        m_passwordEdit->setEnabled(true);
        m_passwordEdit->setFocus();
    }
    if (m_loginButton) m_loginButton->setEnabled(true);
    qDebug() << "管理员登录视图(AdminLoginView): UI已重置为初始状态。";
}

QString AdminLoginView::getPassword() const
{
    if (m_passwordEdit) {
        return m_passwordEdit->text();
    }
    qWarning() << "AdminLoginView: m_passwordEdit is null in getPassword(). Returning empty string.";
    return QString();
}