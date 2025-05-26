/****************************************************************************
** Meta object code from reading C++ file 'AdminModule.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../AdminModule.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdminModule.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN11AdminModuleE_t {};
} // unnamed namespace

template <> constexpr inline auto AdminModule::qt_create_metaobjectdata<qt_meta_tag_ZN11AdminModuleE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AdminModule",
        "adminViewVisible",
        "",
        "visible",
        "exitAdminModeRequested",
        "configurationChanged",
        "loginSuccessfulAndAdminActive",
        "showLoginView",
        "requestExitAdminMode",
        "onAdminLoginHotkeyChanged",
        "QList<DWORD>",
        "newHotkeySequence",
        "isAnyViewVisible",
        "onWhitelistUpdated",
        "QList<AppInfo>",
        "updatedWhitelist",
        "getWhitelistedApps",
        "isLoginViewActive",
        "onUserRequestsExitAdminMode",
        "onLoginViewRequestsExit",
        "onLoginAttempt",
        "password",
        "onChangePasswordRequested",
        "currentPassword",
        "newPassword",
        "onLoginViewHidden"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'adminViewVisible'
        QtMocHelpers::SignalData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Signal 'exitAdminModeRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'configurationChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loginSuccessfulAndAdminActive'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'showLoginView'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'requestExitAdminMode'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onAdminLoginHotkeyChanged'
        QtMocHelpers::SlotData<void(const QList<DWORD> &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'isAnyViewVisible'
        QtMocHelpers::SlotData<bool() const>(12, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'onWhitelistUpdated'
        QtMocHelpers::SlotData<void(const QList<AppInfo> &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'getWhitelistedApps'
        QtMocHelpers::SlotData<QList<AppInfo>() const>(16, 2, QMC::AccessPublic, 0x80000000 | 14),
        // Slot 'isLoginViewActive'
        QtMocHelpers::SlotData<bool() const>(17, 2, QMC::AccessPublic, QMetaType::Bool),
        // Slot 'onUserRequestsExitAdminMode'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLoginViewRequestsExit'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLoginAttempt'
        QtMocHelpers::SlotData<void(const QString &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 21 },
        }}),
        // Slot 'onChangePasswordRequested'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 23 }, { QMetaType::QString, 24 },
        }}),
        // Slot 'onLoginViewHidden'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AdminModule, qt_meta_tag_ZN11AdminModuleE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AdminModule::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11AdminModuleE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11AdminModuleE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11AdminModuleE_t>.metaTypes,
    nullptr
} };

void AdminModule::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AdminModule *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->adminViewVisible((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->exitAdminModeRequested(); break;
        case 2: _t->configurationChanged(); break;
        case 3: _t->loginSuccessfulAndAdminActive(); break;
        case 4: _t->showLoginView(); break;
        case 5: _t->requestExitAdminMode(); break;
        case 6: _t->onAdminLoginHotkeyChanged((*reinterpret_cast< std::add_pointer_t<QList<DWORD>>>(_a[1]))); break;
        case 7: { bool _r = _t->isAnyViewVisible();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->onWhitelistUpdated((*reinterpret_cast< std::add_pointer_t<QList<AppInfo>>>(_a[1]))); break;
        case 9: { QList<AppInfo> _r = _t->getWhitelistedApps();
            if (_a[0]) *reinterpret_cast< QList<AppInfo>*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->isLoginViewActive();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->onUserRequestsExitAdminMode(); break;
        case 12: _t->onLoginViewRequestsExit(); break;
        case 13: _t->onLoginAttempt((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->onChangePasswordRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: _t->onLoginViewHidden(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AdminModule::*)(bool )>(_a, &AdminModule::adminViewVisible, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminModule::*)()>(_a, &AdminModule::exitAdminModeRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminModule::*)()>(_a, &AdminModule::configurationChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminModule::*)()>(_a, &AdminModule::loginSuccessfulAndAdminActive, 3))
            return;
    }
}

const QMetaObject *AdminModule::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdminModule::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11AdminModuleE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AdminModule::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void AdminModule::adminViewVisible(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void AdminModule::exitAdminModeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AdminModule::configurationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AdminModule::loginSuccessfulAndAdminActive()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
