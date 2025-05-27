#ifndef APPCARDWIDGET_H
#define APPCARDWIDGET_H

#include <QWidget>
#include <QString>
#include <QIcon>

class QLabel;
class QVBoxLayout;
class QMovie;

class AppCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AppCardWidget(const QString& appName, const QString& appPath, const QIcon& appIcon, QWidget *parent = nullptr);
    QString getAppPath() const { return m_appPath; }
    QString getAppName() const { return m_appName; }

public slots:
    void setLoadingState(bool loading);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void launchAppRequested(const QString& appPath, const QString& appName);

private:
    void setupUi();

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QVBoxLayout *m_layout;
    QLabel* m_loadingIndicatorLabel;
    QMovie* m_loadingMovie;

    QString m_appName;
    QString m_appPath;
    QIcon m_appIcon;
    bool m_isLoading;
};

#endif // APPCARDWIDGET_H 