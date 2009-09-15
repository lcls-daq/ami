/****************************************************************************
** Meta object code from reading C++ file 'Cursors.hh'
**
** Created: Fri Sep 11 11:06:37 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Cursors.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Cursors.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Cursors[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x05,
      28,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
      38,   17,   17,   17, 0x0a,
      55,   17,   17,   17, 0x0a,
      62,   17,   17,   17, 0x0a,
      75,   17,   17,   17, 0x0a,
      90,   17,   17,   17, 0x0a,
      97,   17,   17,   17, 0x0a,
     104,   17,   17,   17, 0x0a,
     111,   17,   17,   17, 0x0a,
     133,   17,   17,   17, 0x0a,
     147,   17,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Cursors[] = {
    "Ami::Qt::Cursors\0\0changed()\0grabbed()\0"
    "set_channel(int)\0calc()\0add_cursor()\0"
    "hide_cursors()\0plot()\0load()\0save()\0"
    "remove_plot(QObject*)\0grab_cursor()\0"
    "change_features()\0"
};

const QMetaObject Ami::Qt::Cursors::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Cursors,
      qt_meta_data_Ami__Qt__Cursors, 0 }
};

const QMetaObject *Ami::Qt::Cursors::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Cursors::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Cursors))
	return static_cast<void*>(const_cast< Cursors*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Cursors::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: changed(); break;
        case 1: grabbed(); break;
        case 2: set_channel((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: calc(); break;
        case 4: add_cursor(); break;
        case 5: hide_cursors(); break;
        case 6: plot(); break;
        case 7: load(); break;
        case 8: save(); break;
        case 9: remove_plot((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        case 10: grab_cursor(); break;
        case 11: change_features(); break;
        }
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Cursors::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void Ami::Qt::Cursors::grabbed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
