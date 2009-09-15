/****************************************************************************
** Meta object code from reading C++ file 'Transform.hh'
**
** Created: Thu Sep 10 14:58:17 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Transform.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Transform.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Transform[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x05,

 // slots: signature, parameters, type, tag, flags
      30,   19,   19,   19, 0x0a,
      36,   19,   19,   19, 0x0a,
      52,   19,   19,   19, 0x0a,
      59,   19,   19,   19, 0x0a,
      67,   19,   19,   19, 0x0a,
      75,   19,   19,   19, 0x0a,
      82,   19,   19,   19, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Transform[] = {
    "Ami::Qt::Transform\0\0changed()\0add()\0"
    "remove(QString)\0calc()\0apply()\0clear()\0"
    "save()\0load()\0"
};

const QMetaObject Ami::Qt::Transform::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Transform,
      qt_meta_data_Ami__Qt__Transform, 0 }
};

const QMetaObject *Ami::Qt::Transform::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Transform::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Transform))
	return static_cast<void*>(const_cast< Transform*>(this));
    if (!strcmp(_clname, "Ami::AbsTransform"))
	return static_cast< Ami::AbsTransform*>(const_cast< Transform*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Transform::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        }
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Transform::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
