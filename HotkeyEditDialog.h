#ifndef HOTKEYEDITDIALOG_H
#define HOTKEYEDITDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QSet> // To store currently pressed keys
#include <QList> // For QList<KeyEventInfo>
#include <Windows.h> // For DWORD and VK_ codes

// Forward declarations
class QLabel;
class QPushButton;
class QKeyEvent;

class HotkeyEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HotkeyEditDialog(QWidget *parent = nullptr);
    ~HotkeyEditDialog();

    QStringList getSelectedHotkey() const;
    QList<DWORD> getSelectedHotkeyVkCodes() const;

signals:
    void hotkeySelected(const QStringList& hotkey);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void acceptPressed();
    void rejectPressed(); // For cancel

private:
    struct KeyEventInfo {
        DWORD vkCode = 0;
        DWORD scanCode = 0;
        bool isExtendedKey = false;
        QString displayString;
        QString vkString;

        KeyEventInfo() = default;
        KeyEventInfo(DWORD vk, DWORD sc, bool ext, QString disp, QString vk_s)
            : vkCode(vk), scanCode(sc), isExtendedKey(ext), displayString(std::move(disp)), vkString(std::move(vk_s)) {}

        bool operator==(const KeyEventInfo& other) const {
            return vkCode == other.vkCode;
        }
        bool operator==(DWORD otherVkCode) const {
            return vkCode == otherVkCode;
        }
    };

    void setupUi();
    void updateHotkeyDisplay();
    bool isValidCombination(const QList<KeyEventInfo>& keysInfo, QStringList& outDisplayStrings, QStringList& outVkStrings) const;

    QLabel *m_infoLabel;        // E.g., "Press the desired hotkey combination"
    QLabel *m_hotkeyDisplayLabel; // Shows currently pressed keys like "CTRL + SHIFT + A"
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    QList<KeyEventInfo> m_currentlyHeldKeysInfo; // Keys currently held down
    QList<KeyEventInfo> m_recordedKeysInfo;      // Last valid sequence when keys were released

    QStringList m_currentHotkeyVkStrings; // This will be derived from m_currentlyHeldKeysInfo or m_recordedKeysInfo for emitting/saving
                                          // Let's rename this or ensure its role is clear. Maybe this becomes m_finalVkStringsToEmit.

    // Max number of keys in a hotkey sequence (e.g., Ctrl+Shift+Alt+K = 4)
    const int MAX_HOTKEY_COMPONENTS = 4; 
};

#endif // HOTKEYEDITDIALOG_H 