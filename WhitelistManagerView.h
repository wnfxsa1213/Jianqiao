#ifndef WHITELISTMANAGERVIEW_H
#define WHITELISTMANAGERVIEW_H

#include <QWidget>
#include <QList>
#include "common_types.h" // Include the common types header
#include "HotkeyEditDialog.h" // << ADDED
// #include <QStringPair> // Removed, as this header does not exist in standard Qt

// Forward declarations
class QListWidget;
class QPushButton;
class QListWidgetItem;
class QLineEdit; // Added for password change fields
class QGroupBox; // Added for grouping password change UI
class HotkeyEditDialog; // Forward declaration if not including full header (but we are)

// struct WhitelistedApp; // No longer needed here, use AppInfo from common_types.h

namespace Ui {
class WhitelistManagerView;
}

class WhitelistManagerView : public QWidget
{
    Q_OBJECT
public:
    explicit WhitelistManagerView(const QList<AppInfo>& currentWhitelist, QWidget *parent = nullptr);
    ~WhitelistManagerView();

    void populateList(const QList<AppInfo>& apps);
    QList<AppInfo> getCurrentApps() const;

signals:
    void whitelistChanged(const QList<AppInfo>& newWhitelist);
    void userRequestsExitAdminMode();
    void changePasswordRequested(const QString& currentPassword, const QString& newPassword); // New signal
    void adminLoginHotkeyChanged(const QStringList& newHotkeyVkStrings); // << ADDED SIGNAL

private slots:
    void onAddAppClicked();
    void onRemoveAppClicked();
    void onListItemChanged(QListWidgetItem *item); // For potential inline editing in future
    void onChangePasswordClicked(); // New slot
    void onEditHotkeyClicked(); // << ADDED SLOT
    void handleNewAdminHotkeySelected(const QStringList& newVkStrings); // << ADDED SLOT

private:
    Ui::WhitelistManagerView *ui;
    void setupUi();

    QListWidget *m_appListWidget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_secureExitProgramButton;
    //QPushButton *m_saveButton; // Consider if explicit save is better than auto-save on change

    // UI elements for password change
    QGroupBox *m_passwordChangeGroupBox;
    QLineEdit *m_currentPasswordEdit;
    QLineEdit *m_newPasswordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QPushButton *m_changePasswordButton;

    QPushButton *m_editHotkeyButton; // << ADDED BUTTON
};

#endif // WHITELISTMANAGERVIEW_H 