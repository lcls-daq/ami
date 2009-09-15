/****************************************************************************
** Meta object code from reading C++ file 'EdgeFinder.hh'
**
** Created: Mon Sep 14 14:33:37 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "EdgeFinder.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EdgeFinder.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__EdgeFinder[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      21,   20,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
      31,   20,   20,   20, 0x0a,
      48,   20,   20,   20, 0x0a,
      55,   20,   20,   20, 0x0a,
      62,   20,   20,   20, 0x0a,
      69,   20,   20,   20, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__EdgeFinder[] = {
    "Ami::Qt::EdgeFinder\0\0changed()\0"
    "set_channel(int)\0plot()\0load()\0save()\0"
    "remove_plot(QObject*)\0"
};

const QMetaObject Ami::Qt::EdgeFinder::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__EdgeFinder,
      qt_meta_data_Ami__Qt__EdgeFinder, 0 }
};

const QMetaObject *Ami::Qt::EdgeFinder::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::EdgeFinder::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__EdgeFinder))
	return static_cast<void*>(const_cast< EdgeFinder*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::EdgeFinder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: changed(); break;
        case 1: set_channel((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: plot(); break;
        case 3: load(); break;
        case 4: save(); break;
        case 5: remove_plot((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        }
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::EdgeFinder::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
