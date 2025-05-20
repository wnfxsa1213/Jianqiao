#ifndef HOVERICONWIDGET_H
#define HOVERICONWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QPropertyAnimation>
#include <QLabel>
#include <QVBoxLayout>

class HoverIconWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor)

public:
    explicit HoverIconWidget(const QPixmap &icon, const QString &name, const QString &appPath, QWidget *parent = nullptr);
    ~HoverIconWidget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    qreal scaleFactor() const;
    void setScaleFactor(qreal factor);
    QString applicationPath() const;

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void clicked(const QString& appPath);

private:
    void setupAnimation();

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QString m_appName;
    QString m_appPath;
    QPixmap m_originalPixmap;
    
    qreal m_currentScaleFactor;
    QPropertyAnimation *m_scaleAnimation;

    QSize m_baseSize;
    int m_iconSize;
    int m_padding;
};

#endif // HOVERICONWIDGET_H 