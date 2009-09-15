/****************************************************************************
** Meta object code from reading C++ file 'CursorDefinition.hh'
**
** Created: Sat Sep 12 08:39:26 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "CursorDefinition.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CursorDefinition.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__CursorDefinition[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      27,   26,   26,   26, 0x0a,
      46,   26,   26,   26, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__CursorDefinition[] = {
    "Ami::Qt::CursorDefinition\0\0"
    "show_in_plot(bool)\0remove()\0"
};

const QMetaObject Ami::Qt::CursorDefinition::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__CursorDefinition,
      qt_meta_data_Ami__Qt__CursorDefinition, 0 }
};

const QMetaObject *Ami::Qt::CursorDefinition::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::CursorDefinition::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__CursorDefinition))
	return static_cast<void*>(const_cast< CursorDefinition*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::CursorDefinition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: show_in_plot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: remove(); break;
        }
        _id -= 2;
    }
    return _id;
}
