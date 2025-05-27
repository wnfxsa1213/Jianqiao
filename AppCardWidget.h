#ifndef APPCARDWIDGET_H
#define APPCARDWIDGET_H

#include <QWidget>
#include <QString>
#include <QIcon>
#include <QPropertyAnimation>

class QLabel;
class QVBoxLayout;
class QMovie;

class AppCardWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor)
public:
    explicit AppCardWidget(const QString& appName, const QString& appPath, const QIcon& appIcon, QWidget *parent = nullptr);
    QString getAppPath() const { return m_appPath; }
    QString getAppName() const { return m_appName; }

    qreal scaleFactor() const { return m_scaleFactor; }
    void setScaleFactor(qreal factor);

public slots:
    void setLoadingState(bool loading);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void launchAppRequested(const QString& appPath, const QString& appName);

private:
    void setupUi();
    void updateIconPosition();

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QVBoxLayout *m_layout;
    QLabel* m_loadingIndicatorLabel;
    QMovie* m_loadingMovie;

    QPropertyAnimation* m_scaleAnimation;
    qreal m_scaleFactor;
    static constexpr qreal DEFAULT_SCALE = 1.0;
    static constexpr qreal HOVER_SCALE = 1.25;
    static constexpr int ANIMATION_DURATION = 150;

    QString m_appName;
    QString m_appPath;
    QIcon m_appIcon;
    bool m_isLoading;
};

#endif // APPCARDWIDGET_H 