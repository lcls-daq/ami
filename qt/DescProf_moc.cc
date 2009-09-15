/****************************************************************************
** Meta object code from reading C++ file 'DescProf.hh'
**
** Created: Tue Sep 1 14:32:38 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "DescProf.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DescProf.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__DescProf[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      19,   18,   18,   18, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__DescProf[] = {
    "Ami::Qt::DescProf\0\0set_variable(QString)\0"
};

const QMetaObject Ami::Qt::DescProf::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__DescProf,
      qt_meta_data_Ami__Qt__DescProf, 0 }
};

const QMetaObject *Ami::Qt::DescProf::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::DescProf::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__DescProf))
	return static_cast<void*>(const_cast< DescProf*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::DescProf::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: set_variable((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        }
        _id -= 1;
    }
    return _id;
}
