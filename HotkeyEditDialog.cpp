#include "HotkeyEditDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDebug>
#include <QKeySequence>
#include <QMessageBox> // For warnings

// Placeholder for VK to Qt::Key mapping or vice-versa if needed more generically later
// For now, keyEventToVkWString will be expanded in updateHotkeyDisplay's logic directly.

// Helper function to convert VK code to a display string and a VK_ string
// Returns a pair: {display string, VK_ string}
// For modifiers, extended helps distinguish left/right if possible
static QPair<QString, QString> vkToDisplayAndVkString(DWORD vkCode, DWORD scanCode, bool isExtendedKey) {
    // Priority for L/R distinction using MapVirtualKey for Shift, Ctrl, Alt
    // scanCode here is the hardware scan code.
    // isExtendedKey is true if it's an "extended key" (e.g. RAlt, RCtrl, Ins, Del, etc.)
    if (vkCode == VK_SHIFT) {
        UINT mappedVk = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
        if (mappedVk == VK_LSHIFT) return {HotkeyEditDialog::tr("LSHIFT"), "VK_LSHIFT"};
        if (mappedVk == VK_RSHIFT) return {HotkeyEditDialog::tr("RSHIFT"), "VK_RSHIFT"};
        return {HotkeyEditDialog::tr("SHIFT"), "VK_SHIFT"}; // Fallback
    }
    if (vkCode == VK_CONTROL) {
        UINT mappedVk = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
        if (mappedVk == VK_LCONTROL) return {HotkeyEditDialog::tr("LCTRL"), "VK_LCONTROL"};
        if (mappedVk == VK_RCONTROL) return {HotkeyEditDialog::tr("RCTRL"), "VK_RCONTROL"};
        return {HotkeyEditDialog::tr("CTRL"), "VK_CONTROL"}; // Fallback
    }
    if (vkCode == VK_MENU) { // Alt key
        UINT mappedVk = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX); // Use MAPVK_VSC_TO_VK_EX to distinguish L/R Alt
        if (mappedVk == VK_LMENU) return {HotkeyEditDialog::tr("LALT"), "VK_LMENU"};
        if (mappedVk == VK_RMENU) return {HotkeyEditDialog::tr("RALT"), "VK_RMENU"};
        return {HotkeyEditDialog::tr("ALT"), "VK_MENU"}; // Fallback
    }
    if (vkCode == VK_LWIN) return {HotkeyEditDialog::tr("LWIN"), "VK_LWIN"};
    if (vkCode == VK_RWIN) return {HotkeyEditDialog::tr("RWIN"), "VK_RWIN"};

    wchar_t keyNameBuffer[256];
    // Construct the lparam for GetKeyNameText from scanCode and isExtendedKey flag
    // Bit 24 (0x01000000) of lParam for WM_KEYDOWN indicates an extended key.
    LONG lparam = (scanCode << 16) | (isExtendedKey ? (1 << 24) : 0);

    if (GetKeyNameText(lparam, keyNameBuffer, sizeof(keyNameBuffer)/sizeof(wchar_t))) {
        QString keyName = QString::fromWCharArray(keyNameBuffer).toUpper();
        // Common VK codes
        if (vkCode >= 'A' && vkCode <= 'Z') return {keyName, "VK_" + keyName};
        if (vkCode >= '0' && vkCode <= '9') return {keyName, "VK_" + keyName};
        if (vkCode >= VK_F1 && vkCode <= VK_F24) return {keyName, "VK_F" + QString::number(vkCode - VK_F1 + 1)};
        
        switch (vkCode) {
            case VK_SPACE: return {HotkeyEditDialog::tr("SPACE"), "VK_SPACE"};
            case VK_RETURN: return {HotkeyEditDialog::tr("ENTER"), "VK_RETURN"};
            case VK_BACK: return {HotkeyEditDialog::tr("BACKSPACE"), "VK_BACK"};
            case VK_DELETE: return {HotkeyEditDialog::tr("DELETE"), "VK_DELETE"};
            case VK_TAB: return {HotkeyEditDialog::tr("TAB"), "VK_TAB"};
            case VK_ESCAPE: return {HotkeyEditDialog::tr("ESC"), "VK_ESCAPE"};
            case VK_PRIOR: return {HotkeyEditDialog::tr("PAGE UP"), "VK_PRIOR"};
            case VK_NEXT: return {HotkeyEditDialog::tr("PAGE DOWN"), "VK_NEXT"};
            case VK_END: return {HotkeyEditDialog::tr("END"), "VK_END"};
            case VK_HOME: return {HotkeyEditDialog::tr("HOME"), "VK_HOME"};
            case VK_LEFT: return {HotkeyEditDialog::tr("LEFT"), "VK_LEFT"};
            case VK_UP: return {HotkeyEditDialog::tr("UP"), "VK_UP"};
            case VK_RIGHT: return {HotkeyEditDialog::tr("RIGHT"), "VK_RIGHT"};
            case VK_DOWN: return {HotkeyEditDialog::tr("DOWN"), "VK_DOWN"};
            case VK_INSERT: return {HotkeyEditDialog::tr("INSERT"), "VK_INSERT"};
            case VK_CAPITAL: return {HotkeyEditDialog::tr("CAPS LOCK"), "VK_CAPITAL"};
            case VK_NUMLOCK: return {HotkeyEditDialog::tr("NUM LOCK"), "VK_NUMLOCK"};
            case VK_SCROLL: return {HotkeyEditDialog::tr("SCROLL LOCK"), "VK_SCROLL"};
            // OEM keys - these can vary. Using common US layout names.
            case VK_OEM_1: return {HotkeyEditDialog::tr("; :"), "VK_OEM_1"}; // Semicolon
            case VK_OEM_PLUS: return {HotkeyEditDialog::tr("= +"), "VK_OEM_PLUS"};
            case VK_OEM_COMMA: return {HotkeyEditDialog::tr(", <"), "VK_OEM_COMMA"};
            case VK_OEM_MINUS: return {HotkeyEditDialog::tr("- _"), "VK_OEM_MINUS"};
            case VK_OEM_PERIOD: return {HotkeyEditDialog::tr(". >"), "VK_OEM_PERIOD"};
            case VK_OEM_2: return {HotkeyEditDialog::tr("/ ?"), "VK_OEM_2"}; // Forward slash
            case VK_OEM_3: return {HotkeyEditDialog::tr("` ~"), "VK_OEM_3"}; // Grave accent (backtick)
            case VK_OEM_4: return {HotkeyEditDialog::tr("[ {"), "VK_OEM_4"}; // Open bracket
            case VK_OEM_5: return {HotkeyEditDialog::tr("\\\\ |"), "VK_OEM_5"}; // Backslash
            case VK_OEM_6: return {HotkeyEditDialog::tr("] }"), "VK_OEM_6"}; // Close bracket
            case VK_OEM_7: return {HotkeyEditDialog::tr("Quote"), "VK_OEM_7"}; // Single quote / Double quote (using descriptive text)
            // Numpad keys
            case VK_NUMPAD0: return {HotkeyEditDialog::tr("NUM 0"), "VK_NUMPAD0"};
            case VK_NUMPAD1: return {HotkeyEditDialog::tr("NUM 1"), "VK_NUMPAD1"};
            case VK_NUMPAD2: return {HotkeyEditDialog::tr("NUM 2"), "VK_NUMPAD2"};
            case VK_NUMPAD3: return {HotkeyEditDialog::tr("NUM 3"), "VK_NUMPAD3"};
            case VK_NUMPAD4: return {HotkeyEditDialog::tr("NUM 4"), "VK_NUMPAD4"};
            case VK_NUMPAD5: return {HotkeyEditDialog::tr("NUM 5"), "VK_NUMPAD5"};
            case VK_NUMPAD6: return {HotkeyEditDialog::tr("NUM 6"), "VK_NUMPAD6"};
            case VK_NUMPAD7: return {HotkeyEditDialog::tr("NUM 7"), "VK_NUMPAD7"};
            case VK_NUMPAD8: return {HotkeyEditDialog::tr("NUM 8"), "VK_NUMPAD8"};
            case VK_NUMPAD9: return {HotkeyEditDialog::tr("NUM 9"), "VK_NUMPAD9"};
            case VK_MULTIPLY: return {HotkeyEditDialog::tr("NUM *"), "VK_MULTIPLY"};
            case VK_ADD: return {HotkeyEditDialog::tr("NUM +"), "VK_ADD"};
            case VK_SUBTRACT: return {HotkeyEditDialog::tr("NUM -"), "VK_SUBTRACT"};
            case VK_DECIMAL: return {HotkeyEditDialog::tr("NUM ."), "VK_DECIMAL"};
            case VK_DIVIDE: return {HotkeyEditDialog::tr("NUM /"), "VK_DIVIDE"};
            default:
                if (!keyName.isEmpty()) return {keyName, "VK_UNKNOWN_PLEASE_SPECIFY_" + keyName}; // Fallback to display name
                qDebug() << "vkToDisplayAndVkString: Unhandled VK Code:" << vkCode << "ScanCode:" << scanCode;
                return {QString("UNK:%1").arg(vkCode), QString("VK_GENERIC_%1").arg(vkCode)};
        }
    }
    // Fallback if GetKeyNameText fails
    qDebug() << "vkToDisplayAndVkString: GetKeyNameText failed for VK Code:" << vkCode << "ScanCode:" << scanCode;
    return {QString("V%1").arg(vkCode), QString("VK_RAW_%1").arg(vkCode)};
}

HotkeyEditDialog::HotkeyEditDialog(QWidget *parent)
    : QDialog(parent)
{
    qDebug() << "HotkeyEditDialog: Constructor called.";
    setupUi();
    setWindowTitle(tr("设置新的快捷键"));
    setModal(true);
}

HotkeyEditDialog::~HotkeyEditDialog()
{
    qDebug() << "HotkeyEditDialog销毁";
}

void HotkeyEditDialog::setupUi()
{
    m_infoLabel = new QLabel(tr("请按下您想设置的组合键，然后点击\"确定\"。\n(例如: CTRL + SHIFT + A)\n(最多包含 %1 个按键)").arg(MAX_HOTKEY_COMPONENTS), this);
    m_infoLabel->setWordWrap(true);

    m_hotkeyDisplayLabel = new QLabel(QString("[%1]").arg(tr("尚未录制按键")), this);
    m_hotkeyDisplayLabel->setAlignment(Qt::AlignCenter);
    m_hotkeyDisplayLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_hotkeyDisplayLabel->setMinimumHeight(30);
    m_hotkeyDisplayLabel->setStyleSheet("QLabel { font-weight: bold; padding: 5px; }");

    m_okButton = new QPushButton(tr("确定"), this);
    m_cancelButton = new QPushButton(tr("取消"), this);

    connect(m_okButton, &QPushButton::clicked, this, &HotkeyEditDialog::acceptPressed);
    connect(m_cancelButton, &QPushButton::clicked, this, &HotkeyEditDialog::rejectPressed);
    m_okButton->setEnabled(false); // Initially disabled until a valid hotkey is pressed

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_hotkeyDisplayLabel, 1); // Add stretch factor
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    // Request focus to ensure key events are received.
    // This is important because QDialogs might not grab focus by default in all scenarios.
    // Forcing focus grab is more robust.
}

void HotkeyEditDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    DWORD vkCode = event->nativeVirtualKey();
    DWORD scanCode = event->nativeScanCode();
    bool isExtended = false;

    // Determine if it's an extended key based on VK code.
    // This list covers common extended keys.
    if ((vkCode == VK_RMENU) || (vkCode == VK_RCONTROL) ||
        (vkCode == VK_INSERT) || (vkCode == VK_DELETE) || (vkCode == VK_HOME) || (vkCode == VK_END) ||
        (vkCode == VK_PRIOR) || (vkCode == VK_NEXT) || (vkCode == VK_LEFT) || (vkCode == VK_UP) ||
        (vkCode == VK_RIGHT) || (vkCode == VK_DOWN) || (vkCode == VK_NUMLOCK) || // NumLock is often extended
        (vkCode == VK_SNAPSHOT) || // PrintScreen
        (vkCode == VK_CANCEL) || // Break key
        (vkCode == VK_RETURN && HIBYTE(LOWORD(GetKeyState(VK_CONTROL))))) { // Numpad Enter is often VK_RETURN with extended flag
        // A more reliable way for Numpad Enter might be checking event->modifiers() & Qt::KeypadModifier
        // if Qt sets it reliably for Numpad Enter when it sends VK_RETURN.
        // For now, this is an approximation. The key thing is that scanCode and isExtendedKey are now passed to vkToDisplayAndVkString.
        isExtended = true;
    }
    // For VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU specifically,
    // vkToDisplayAndVkString uses MapVirtualKey with scanCode to distinguish them, so isExtended might be less critical for these
    // if Qt already gives distinct L/R virtual key codes for them.


    if (vkCode == 0) { 
        event->accept();
        return;
    }

    // Check if key already pressed
    bool alreadyPressed = false;
    for (const auto& keyInfo : m_currentlyHeldKeysInfo) {
        if (keyInfo.vkCode == vkCode) {
            alreadyPressed = true;
            break;
        }
    }

    if (alreadyPressed) {
        event->accept();
        return;
    }

    if (m_currentlyHeldKeysInfo.size() >= MAX_HOTKEY_COMPONENTS) {
        qDebug() << "HotkeyEditDialog: Max components reached, not adding VK:" << vkCode;
        event->accept();
        return;
    }
    
    // Clear previously recorded sequence as a new key press starts a new interaction cycle
    if (!m_recordedKeysInfo.isEmpty()) {
        m_recordedKeysInfo.clear();
    }

    QPair<QString, QString> keyStrings = vkToDisplayAndVkString(vkCode, scanCode, isExtended);
    m_currentlyHeldKeysInfo.append(KeyEventInfo(vkCode, scanCode, isExtended, keyStrings.first, keyStrings.second));
    
    updateHotkeyDisplay();
    event->accept();
}

void HotkeyEditDialog::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    DWORD vkCode = event->nativeVirtualKey();
    if (vkCode == 0) {
        event->accept();
        return;
    }

    // Store a copy of the currently held keys *before* removing the released key.
    // This represents the combination that was active just before this key was released.
    QList<KeyEventInfo> potentialHotkeySequence = m_currentlyHeldKeysInfo;

    bool foundAndRemoved = false;
    for (int i = 0; i < m_currentlyHeldKeysInfo.size(); ++i) {
        if (m_currentlyHeldKeysInfo[i].vkCode == vkCode) {
            m_currentlyHeldKeysInfo.removeAt(i);
            foundAndRemoved = true;
            break;
        }
    }

    if (!foundAndRemoved) {
        // Key was not in our list, weird. Or already processed.
        event->accept();
        return;
    }
    
    // If all keys that were part of the sequence are now released
    if (m_currentlyHeldKeysInfo.isEmpty() && !potentialHotkeySequence.isEmpty()) {
        // This means `potentialHotkeySequence` was the combination when the last key was released.
        // We should check if this `potentialHotkeySequence` is valid.
        QStringList displayStrings, vkStrings;
        if (isValidCombination(potentialHotkeySequence, displayStrings, vkStrings)) {
            m_recordedKeysInfo = potentialHotkeySequence; // Record this valid sequence
            qDebug() << "HotkeyEditDialog: Hotkey sequence recorded:" << vkStrings.join(" + ");
        } else {
            qDebug() << "HotkeyEditDialog: Released sequence was not valid, not recording.";
             // Optionally clear m_recordedKeysInfo if an invalid sequence release should clear old valid ones
            m_recordedKeysInfo.clear(); 
        }
    }
    // If some keys are still held, m_currentlyHeldKeysInfo is not empty.
    // updateHotkeyDisplay will handle showing the current state or the last recorded state.
    
    updateHotkeyDisplay();
    event->accept();
}

// Helper function to check if a list of KeyEventInfo forms a valid hotkey combination
bool HotkeyEditDialog::isValidCombination(const QList<KeyEventInfo>& keysInfo, QStringList& outDisplayStrings, QStringList& outVkStrings) const {
    outDisplayStrings.clear();
    outVkStrings.clear();

    if (keysInfo.isEmpty() || keysInfo.size() > MAX_HOTKEY_COMPONENTS) {
        return false;
    }

    bool hasNonModifierComponent = false;
    int nonModifierCount = 0;

    // Temporary list to sort for consistent display order (e.g., CTRL, SHIFT, ALT, Other)
    QList<KeyEventInfo> sortedKeyInfos = keysInfo;
    std::sort(sortedKeyInfos.begin(), sortedKeyInfos.end(), [](const KeyEventInfo& a, const KeyEventInfo& b) {
        auto isModifier = [](DWORD vk) {
            return (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
                    vk == VK_SHIFT   || vk == VK_LSHIFT   || vk == VK_RSHIFT ||
                    vk == VK_MENU    || vk == VK_LMENU    || vk == VK_RMENU ||
                    vk == VK_LWIN    || vk == VK_RWIN);
        };
        bool aIsMod = isModifier(a.vkCode);
        bool bIsMod = isModifier(b.vkCode);
        if (aIsMod && !bIsMod) return true;
        if (!aIsMod && bIsMod) return false;
        // Optional: Further sort modifiers (e.g., CTRL > SHIFT > ALT) or non-modifiers alphabetically
        return a.vkCode < b.vkCode;
    });

    QSet<QString> addedVkStringsSet; // To avoid duplicate VK_ strings

    for (const auto& keyInfo : sortedKeyInfos) {
        if (!keyInfo.displayString.isEmpty()) {
            outDisplayStrings << keyInfo.displayString;
        }
        if (!keyInfo.vkString.isEmpty() && !addedVkStringsSet.contains(keyInfo.vkString)) {
            outVkStrings << keyInfo.vkString;
            addedVkStringsSet.insert(keyInfo.vkString);
        }

        if (!(keyInfo.vkString.startsWith("VK_LCONTROL") || keyInfo.vkString.startsWith("VK_RCONTROL") || keyInfo.vkString == "VK_CONTROL" ||
              keyInfo.vkString.startsWith("VK_LSHIFT") || keyInfo.vkString.startsWith("VK_RSHIFT") || keyInfo.vkString == "VK_SHIFT" ||
              keyInfo.vkString.startsWith("VK_LMENU") || keyInfo.vkString.startsWith("VK_RMENU") || keyInfo.vkString == "VK_MENU" ||
              keyInfo.vkString.startsWith("VK_LWIN") || keyInfo.vkString.startsWith("VK_RWIN"))) {
            if (!keyInfo.vkString.contains("UNKNOWN") && !keyInfo.vkString.contains("RAW") && !keyInfo.vkString.contains("GENERIC")) {
                nonModifierCount++;
                hasNonModifierComponent = true;
            }
        }
    }

    if (outVkStrings.isEmpty() || outVkStrings.size() > MAX_HOTKEY_COMPONENTS) {
        return false; // Should have been caught by keysInfo.size() check, but good to be safe
    }
    
    bool combinationValid = hasNonModifierComponent && nonModifierCount >= 1;

    // Special case: allow single F-keys or special keys (Esc, Del, etc.) without modifiers.
    if (!hasNonModifierComponent && outVkStrings.size() == 1) {
        const QString& singleKeyVkStr = outVkStrings.first();
        if (singleKeyVkStr.startsWith("VK_F") || singleKeyVkStr == "VK_ESCAPE" || singleKeyVkStr == "VK_DELETE" ||
            singleKeyVkStr == "VK_INSERT" || singleKeyVkStr == "VK_HOME" || singleKeyVkStr == "VK_END" ||
            singleKeyVkStr == "VK_PRIOR" || singleKeyVkStr == "VK_NEXT" || singleKeyVkStr == "VK_PAUSE" /* VK_PAUSE is 0x13 */ ||
            singleKeyVkStr == "VK_SCROLL" /* VK_SCROLL is 0x91 */ || singleKeyVkStr == "VK_PRINT" /* VK_SNAPSHOT is 0x2C */) {
            combinationValid = true;
        }
    }
    return combinationValid;
}

void HotkeyEditDialog::updateHotkeyDisplay()
{
    QStringList displayStrings;
    QStringList vkStrings; // For validation and potentially enabling OK button
    bool currentCombinationIsValid = false;

    if (!m_currentlyHeldKeysInfo.isEmpty()) {
        // User is actively pressing keys
        currentCombinationIsValid = isValidCombination(m_currentlyHeldKeysInfo, displayStrings, vkStrings);
        if (!displayStrings.isEmpty()) {
            m_hotkeyDisplayLabel->setText(displayStrings.join(" + "));
        } else {
            m_hotkeyDisplayLabel->setText("[" + tr("正在录制...") + "]");
        }
    } else if (!m_recordedKeysInfo.isEmpty()) {
        // User has released keys, and a valid sequence was recorded
        currentCombinationIsValid = isValidCombination(m_recordedKeysInfo, displayStrings, vkStrings); // Re-validate for safety and to get strings
        if (!displayStrings.isEmpty()) {
            m_hotkeyDisplayLabel->setText(displayStrings.join(" + "));
        } else {
             // This case should ideally not happen if m_recordedKeysInfo is valid and populated
            m_hotkeyDisplayLabel->setText("[" + tr("请重新录制") + "]");
        }
    } else {
        // No keys held, nothing recorded
        m_hotkeyDisplayLabel->setText("[" + tr("尚未录制按键") + "]");
    }
    
    m_okButton->setEnabled(currentCombinationIsValid);
}

void HotkeyEditDialog::acceptPressed()
{
    QStringList finalDisplayStrings, finalVkStrings;
    bool canAccept = false;

    if (!m_currentlyHeldKeysInfo.isEmpty()) { // Prefer currently held if user presses OK while holding
        canAccept = isValidCombination(m_currentlyHeldKeysInfo, finalDisplayStrings, finalVkStrings);
    } else if (!m_recordedKeysInfo.isEmpty()) { // Fallback to last recorded valid sequence
        canAccept = isValidCombination(m_recordedKeysInfo, finalDisplayStrings, finalVkStrings);
    }

    if (!canAccept || finalVkStrings.isEmpty()) { 
        QMessageBox::warning(this, tr("无效热键"), tr("请录制一个有效的热键组合 (例如: CTRL + A, 或 F5)。\n组合中必须包含至少一个非修饰键 (如字母、数字、F键等)，且总按键数不超过 %1。").arg(MAX_HOTKEY_COMPONENTS));
        return;
    }

    qDebug() << "HotkeyEditDialog: 热键已选择 (VK Strings):" << finalVkStrings.join(" + ");
    qDebug() << "HotkeyEditDialog: 热键已选择 (Display Strings):" << finalDisplayStrings.join(" + ");
    
    emit hotkeySelected(finalVkStrings); // Emit the VK_ strings
    accept();
}

void HotkeyEditDialog::rejectPressed()
{
    qDebug() << "HotkeyEditDialog: 热键选择已取消。";
    reject();
}

QStringList HotkeyEditDialog::getSelectedHotkey() const
{
    // This should return the VK strings of the accepted hotkey
    QStringList displayStrings, vkStrings;
    if (!m_recordedKeysInfo.isEmpty() && isValidCombination(m_recordedKeysInfo, displayStrings, vkStrings)) {
        return vkStrings;
    }
    // Fallback or if called before accept, maybe return empty or based on m_currentlyHeldKeysInfo
    // For simplicity, assume it's called after a successful accept which populates m_recordedKeysInfo
    // However, the acceptPressed now directly uses the derived finalVkStrings for the signal.
    // This function might be less critical if the signal carries all needed info.
    // For now, let's make it consistent with what would be accepted.
    if (!m_currentlyHeldKeysInfo.isEmpty() && isValidCombination(m_currentlyHeldKeysInfo, displayStrings, vkStrings)) {
         return vkStrings; // If user is holding a valid combo
    }
    if (!m_recordedKeysInfo.isEmpty()) { // If a valid combo was released and recorded
        isValidCombination(m_recordedKeysInfo, displayStrings, vkStrings); // to populate vkStrings
        return vkStrings;
    }
    return QStringList(); // Empty if nothing valid is available
}

QList<DWORD> HotkeyEditDialog::getSelectedHotkeyVkCodes() const
{
    QList<DWORD> vkCodes;
    const QList<KeyEventInfo>* keyInfoListToUse = nullptr;

    if (!m_currentlyHeldKeysInfo.isEmpty()) { // Prioritize active held keys if valid
        QStringList dummyDisplay, dummyVk;
        if (isValidCombination(m_currentlyHeldKeysInfo, dummyDisplay, dummyVk)) {
            keyInfoListToUse = &m_currentlyHeldKeysInfo;
        }
    }
    
    if (!keyInfoListToUse && !m_recordedKeysInfo.isEmpty()) { // Fallback to recorded
         QStringList dummyDisplay, dummyVk;
        if (isValidCombination(m_recordedKeysInfo, dummyDisplay, dummyVk)) {
            keyInfoListToUse = &m_recordedKeysInfo;
        }
    }

    if (keyInfoListToUse) {
        for(const auto& keyInfo : *keyInfoListToUse) {
            vkCodes.append(keyInfo.vkCode);
        }
    }
    
    // Optionally sort them here if a consistent order is needed by the caller.
    std::sort(vkCodes.begin(), vkCodes.end());
    return vkCodes;
}

// Override closeEvent to ensure keyboard is released
void HotkeyEditDialog::closeEvent(QCloseEvent *event)
{
    QDialog::closeEvent(event);
}

// Override hideEvent for the same reason (e.g. if closed via other means)
void HotkeyEditDialog::hideEvent(QHideEvent *event)
{
    QDialog::hideEvent(event);
} 