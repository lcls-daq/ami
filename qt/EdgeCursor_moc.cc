/****************************************************************************
** Meta object code from reading C++ file 'EdgeCursor.hh'
**
** Created: Mon Sep 14 14:31:20 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "EdgeCursor.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EdgeCursor.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__EdgeCursor[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      21,   20,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
      31,   20,   20,   20, 0x0a,
      43,   20,   20,   20, 0x0a,
      50,   20,   20,   20, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__EdgeCursor[] = {
    "Ami::Qt::EdgeCursor\0\0changed()\0"
    "set_value()\0grab()\0show_in_plot(bool)\0"
};

const QMetaObject Ami::Qt::EdgeCursor::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__EdgeCursor,
      qt_meta_data_Ami__Qt__EdgeCursor, 0 }
};

const QMetaObject *Ami::Qt::EdgeCursor::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::EdgeCursor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__EdgeCursor))
	return static_cast<void*>(const_cast< EdgeCursor*>(this));
    if (!strcmp(_clname, "Cursors"))
	return static_cast< Cursors*>(const_cast< EdgeCursor*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::EdgeCursor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: changed(); break;
        case 1: set_value(); break;
        case 2: grab(); break;
        case 3: show_in_plot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        }
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::EdgeCursor::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
