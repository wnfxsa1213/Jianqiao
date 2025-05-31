/****************************************************************************
** Meta object code from reading C++ file 'UserModeModule.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../UserModeModule.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UserModeModule.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14UserModeModuleE_t {};
} // unnamed namespace

template <> constexpr inline auto UserModeModule::qt_create_metaobjectdata<qt_meta_tag_ZN14UserModeModuleE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "UserModeModule",
        "userModeActivated",
        "",
        "userModeDeactivated",
        "applicationFailedToLaunch",
        "appName",
        "error",
        "userAppListUpdated",
        "QList<AppInfo>",
        "apps",
        "onApplicationLaunchRequested",
        "appPath",
        "onProcessStateChanged",
        "QProcess::ProcessState",
        "newState",
        "onApplicationActivated",
        "onApplicationActivationFailed",
        "onProcessStarted",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "onProcessError",
        "QProcess::ProcessError"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'userModeActivated'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'userModeDeactivated'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'applicationFailedToLaunch'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 }, { QMetaType::QString, 6 },
        }}),
        // Signal 'userAppListUpdated'
        QtMocHelpers::SignalData<void(const QList<AppInfo> &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Slot 'onApplicationLaunchRequested'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 }, { QMetaType::QString, 5 },
        }}),
        // Slot 'onProcessStateChanged'
        QtMocHelpers::SlotData<void(QProcess::ProcessState)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onApplicationActivated'
        QtMocHelpers::SlotData<void(const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Slot 'onApplicationActivationFailed'
        QtMocHelpers::SlotData<void(const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Slot 'onProcessStarted'
        QtMocHelpers::SlotData<void(const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(const QString &, int, QProcess::ExitStatus)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 }, { QMetaType::Int, 19 }, { 0x80000000 | 20, 21 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(const QString &, QProcess::ProcessError)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 }, { 0x80000000 | 23, 6 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<UserModeModule, qt_meta_tag_ZN14UserModeModuleE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject UserModeModule::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14UserModeModuleE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14UserModeModuleE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14UserModeModuleE_t>.metaTypes,
    nullptr
} };

void UserModeModule::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UserModeModule *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->userModeActivated(); break;
        case 1: _t->userModeDeactivated(); break;
        case 2: _t->applicationFailedToLaunch((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->userAppListUpdated((*reinterpret_cast< std::add_pointer_t<QList<AppInfo>>>(_a[1]))); break;
        case 4: _t->onApplicationLaunchRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->onProcessStateChanged((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessState>>(_a[1]))); break;
        case 6: _t->onApplicationActivated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onApplicationActivationFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onProcessStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[3]))); break;
        case 10: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (UserModeModule::*)()>(_a, &UserModeModule::userModeActivated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (UserModeModule::*)()>(_a, &UserModeModule::userModeDeactivated, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (UserModeModule::*)(const QString & , const QString & )>(_a, &UserModeModule::applicationFailedToLaunch, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (UserModeModule::*)(const QList<AppInfo> & )>(_a, &UserModeModule::userAppListUpdated, 3))
            return;
    }
}

const QMetaObject *UserModeModule::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UserModeModule::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14UserModeModuleE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UserModeModule::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void UserModeModule::userModeActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UserModeModule::userModeDeactivated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UserModeModule::applicationFailedToLaunch(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void UserModeModule::userAppListUpdated(const QList<AppInfo> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
