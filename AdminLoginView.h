#ifndef ADMINLOGINVIEW_H
#define ADMINLOGINVIEW_H

#include <QWidget>

class QLabel; // Forward declaration
class QPushButton; // Forward declaration
class QLineEdit; // Forward declaration for password input

class AdminLoginView : public QWidget
{
    Q_OBJECT
public:
    explicit AdminLoginView(QWidget *parent = nullptr);
    ~AdminLoginView();

    QString getPassword() const;
    void resetUI();

public slots:
    void notifyLoginResult(bool success); // To receive login result from AdminModule

signals:
    void loginAttempt(const QString& password); // Emitted when login button is clicked
    void userRequestsExit();
    void viewHidden();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupUi();
    QLabel *m_infoLabel;
    QLineEdit *m_passwordEdit; // For password input
    QPushButton *m_loginButton; // Login button
    QPushButton *m_exitButton;
};

#endif // ADMINLOGINVIEW_H 