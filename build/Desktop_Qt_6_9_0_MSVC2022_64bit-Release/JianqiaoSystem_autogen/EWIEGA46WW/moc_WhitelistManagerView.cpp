/****************************************************************************
** Meta object code from reading C++ file 'WhitelistManagerView.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../WhitelistManagerView.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WhitelistManagerView.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20WhitelistManagerViewE_t {};
} // unnamed namespace

template <> constexpr inline auto WhitelistManagerView::qt_create_metaobjectdata<qt_meta_tag_ZN20WhitelistManagerViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "WhitelistManagerView",
        "whitelistChanged",
        "",
        "QList<AppInfo>",
        "newWhitelist",
        "userRequestsExitAdminMode",
        "changePasswordRequested",
        "currentPassword",
        "newPassword",
        "adminLoginHotkeyChanged",
        "newHotkeyVkStrings",
        "onAddAppClicked",
        "onRemoveAppClicked",
        "onListItemChanged",
        "QListWidgetItem*",
        "item",
        "onChangePasswordClicked",
        "onEditHotkeyClicked",
        "handleNewAdminHotkeySelected",
        "newVkStrings"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'whitelistChanged'
        QtMocHelpers::SignalData<void(const QList<AppInfo> &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'userRequestsExitAdminMode'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'changePasswordRequested'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'adminLoginHotkeyChanged'
        QtMocHelpers::SignalData<void(const QStringList &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 10 },
        }}),
        // Slot 'onAddAppClicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveAppClicked'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onListItemChanged'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'onChangePasswordClicked'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onEditHotkeyClicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleNewAdminHotkeySelected'
        QtMocHelpers::SlotData<void(const QStringList &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QStringList, 19 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<WhitelistManagerView, qt_meta_tag_ZN20WhitelistManagerViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject WhitelistManagerView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20WhitelistManagerViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20WhitelistManagerViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20WhitelistManagerViewE_t>.metaTypes,
    nullptr
} };

void WhitelistManagerView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<WhitelistManagerView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->whitelistChanged((*reinterpret_cast< std::add_pointer_t<QList<AppInfo>>>(_a[1]))); break;
        case 1: _t->userRequestsExitAdminMode(); break;
        case 2: _t->changePasswordRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->adminLoginHotkeyChanged((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 4: _t->onAddAppClicked(); break;
        case 5: _t->onRemoveAppClicked(); break;
        case 6: _t->onListItemChanged((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 7: _t->onChangePasswordClicked(); break;
        case 8: _t->onEditHotkeyClicked(); break;
        case 9: _t->handleNewAdminHotkeySelected((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (WhitelistManagerView::*)(const QList<AppInfo> & )>(_a, &WhitelistManagerView::whitelistChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (WhitelistManagerView::*)()>(_a, &WhitelistManagerView::userRequestsExitAdminMode, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (WhitelistManagerView::*)(const QString & , const QString & )>(_a, &WhitelistManagerView::changePasswordRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (WhitelistManagerView::*)(const QStringList & )>(_a, &WhitelistManagerView::adminLoginHotkeyChanged, 3))
            return;
    }
}

const QMetaObject *WhitelistManagerView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WhitelistManagerView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20WhitelistManagerViewE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int WhitelistManagerView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void WhitelistManagerView::whitelistChanged(const QList<AppInfo> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void WhitelistManagerView::userRequestsExitAdminMode()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void WhitelistManagerView::changePasswordRequested(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void WhitelistManagerView::adminLoginHotkeyChanged(const QStringList & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
