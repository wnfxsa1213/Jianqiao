/****************************************************************************
** Meta object code from reading C++ file 'AdminDashboardView.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../AdminDashboardView.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdminDashboardView.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18AdminDashboardViewE_t {};
} // unnamed namespace

template <> constexpr inline auto AdminDashboardView::qt_create_metaobjectdata<qt_meta_tag_ZN18AdminDashboardViewE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AdminDashboardView",
        "userRequestsExitAdminMode",
        "",
        "whitelistChanged",
        "QList<AppInfo>",
        "updatedApps",
        "changePasswordRequested",
        "currentPassword",
        "newPassword",
        "adminLoginHotkeyChanged",
        "QList<DWORD>",
        "newHotkeySequence",
        "detectionResultsReceived",
        "SuggestedWindowHints",
        "hints",
        "success",
        "errorString",
        "onAddAppClicked",
        "onRemoveAppClicked",
        "onDetectAndAddAppClicked",
        "onChangeHotkeyClicked",
        "onExitApplicationClicked",
        "onConfirmPasswordChangeClicked",
        "onDetectionResultsReceived",
        "onDetectionDialogApplied",
        "finalMainExecutableHint",
        "finalWindowHints",
        "onDetectionWaitMsSaveClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'userRequestsExitAdminMode'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'whitelistChanged'
        QtMocHelpers::SignalData<void(const QList<AppInfo> &)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'changePasswordRequested'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'adminLoginHotkeyChanged'
        QtMocHelpers::SignalData<void(const QList<DWORD> &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'detectionResultsReceived'
        QtMocHelpers::SignalData<void(const SuggestedWindowHints &, bool, const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 }, { QMetaType::Bool, 15 }, { QMetaType::QString, 16 },
        }}),
        // Slot 'onAddAppClicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveAppClicked'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDetectAndAddAppClicked'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onChangeHotkeyClicked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExitApplicationClicked'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConfirmPasswordChangeClicked'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDetectionResultsReceived'
        QtMocHelpers::SlotData<void(const SuggestedWindowHints &, bool, const QString &)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 }, { QMetaType::Bool, 15 }, { QMetaType::QString, 16 },
        }}),
        // Slot 'onDetectionDialogApplied'
        QtMocHelpers::SlotData<void(const QString &, const QJsonObject &)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 25 }, { QMetaType::QJsonObject, 26 },
        }}),
        // Slot 'onDetectionWaitMsSaveClicked'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AdminDashboardView, qt_meta_tag_ZN18AdminDashboardViewE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AdminDashboardView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18AdminDashboardViewE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18AdminDashboardViewE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18AdminDashboardViewE_t>.metaTypes,
    nullptr
} };

void AdminDashboardView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AdminDashboardView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->userRequestsExitAdminMode(); break;
        case 1: _t->whitelistChanged((*reinterpret_cast< std::add_pointer_t<QList<AppInfo>>>(_a[1]))); break;
        case 2: _t->changePasswordRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->adminLoginHotkeyChanged((*reinterpret_cast< std::add_pointer_t<QList<DWORD>>>(_a[1]))); break;
        case 4: _t->detectionResultsReceived((*reinterpret_cast< std::add_pointer_t<SuggestedWindowHints>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 5: _t->onAddAppClicked(); break;
        case 6: _t->onRemoveAppClicked(); break;
        case 7: _t->onDetectAndAddAppClicked(); break;
        case 8: _t->onChangeHotkeyClicked(); break;
        case 9: _t->onExitApplicationClicked(); break;
        case 10: _t->onConfirmPasswordChangeClicked(); break;
        case 11: _t->onDetectionResultsReceived((*reinterpret_cast< std::add_pointer_t<SuggestedWindowHints>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 12: _t->onDetectionDialogApplied((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 13: _t->onDetectionWaitMsSaveClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< SuggestedWindowHints >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< SuggestedWindowHints >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AdminDashboardView::*)()>(_a, &AdminDashboardView::userRequestsExitAdminMode, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminDashboardView::*)(const QList<AppInfo> & )>(_a, &AdminDashboardView::whitelistChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminDashboardView::*)(const QString & , const QString & )>(_a, &AdminDashboardView::changePasswordRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminDashboardView::*)(const QList<DWORD> & )>(_a, &AdminDashboardView::adminLoginHotkeyChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminDashboardView::*)(const SuggestedWindowHints & , bool , const QString & )>(_a, &AdminDashboardView::detectionResultsReceived, 4))
            return;
    }
}

const QMetaObject *AdminDashboardView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdminDashboardView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18AdminDashboardViewE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int AdminDashboardView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void AdminDashboardView::userRequestsExitAdminMode()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AdminDashboardView::whitelistChanged(const QList<AppInfo> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void AdminDashboardView::changePasswordRequested(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void AdminDashboardView::adminLoginHotkeyChanged(const QList<DWORD> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void AdminDashboardView::detectionResultsReceived(const SuggestedWindowHints & _t1, bool _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
