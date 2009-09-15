/****************************************************************************
** Meta object code from reading C++ file 'Condition.hh'
**
** Created: Tue Sep 8 14:38:38 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Condition.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Condition.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Condition[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x05,

 // slots: signature, parameters, type, tag, flags
      37,   19,   19,   19, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Condition[] = {
    "Ami::Qt::Condition\0\0removed(QString)\0"
    "remove()\0"
};

const QMetaObject Ami::Qt::Condition::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Condition,
      qt_meta_data_Ami__Qt__Condition, 0 }
};

const QMetaObject *Ami::Qt::Condition::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Condition::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Condition))
	return static_cast<void*>(const_cast< Condition*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Condition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: removed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: remove(); break;
        }
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Condition::removed(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
