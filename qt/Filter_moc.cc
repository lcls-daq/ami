/****************************************************************************
** Meta object code from reading C++ file 'Filter.hh'
**
** Created: Thu Sep 10 13:14:40 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Filter.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Filter.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Filter[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x05,

 // slots: signature, parameters, type, tag, flags
      27,   16,   16,   16, 0x0a,
      33,   16,   16,   16, 0x0a,
      49,   16,   16,   16, 0x0a,
      56,   16,   16,   16, 0x0a,
      64,   16,   16,   16, 0x0a,
      72,   16,   16,   16, 0x0a,
      79,   16,   16,   16, 0x0a,
      86,   16,   16,   16, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Filter[] = {
    "Ami::Qt::Filter\0\0changed()\0add()\0"
    "remove(QString)\0calc()\0apply()\0clear()\0"
    "save()\0load()\0update_features()\0"
};

const QMetaObject Ami::Qt::Filter::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Filter,
      qt_meta_data_Ami__Qt__Filter, 0 }
};

const QMetaObject *Ami::Qt::Filter::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Filter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Filter))
	return static_cast<void*>(const_cast< Filter*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Filter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: changed(); break;
        case 1: add(); break;
        case 2: remove((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: calc(); break;
        case 4: apply(); break;
        case 5: clear(); break;
        case 6: save(); break;
        case 7: load(); break;
        case 8: update_features(); break;
        }
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Filter::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
