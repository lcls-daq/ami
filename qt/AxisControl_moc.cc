/****************************************************************************
** Meta object code from reading C++ file 'AxisControl.hh'
**
** Created: Tue Sep 1 14:32:38 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "AxisControl.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AxisControl.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__AxisControl[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
      38,   21,   21,   21, 0x0a,
      60,   21,   21,   21, 0x0a,
      82,   21,   21,   21, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__AxisControl[] = {
    "Ami::Qt::AxisControl\0\0windowChanged()\0"
    "changeLoEdge(QString)\0changeHiEdge(QString)\0"
    "auto_scale(bool)\0"
};

const QMetaObject Ami::Qt::AxisControl::staticMetaObject = {
    { &QGroupBox::staticMetaObject, qt_meta_stringdata_Ami__Qt__AxisControl,
      qt_meta_data_Ami__Qt__AxisControl, 0 }
};

const QMetaObject *Ami::Qt::AxisControl::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::AxisControl::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__AxisControl))
	return static_cast<void*>(const_cast< AxisControl*>(this));
    return QGroupBox::qt_metacast(_clname);
}

int Ami::Qt::AxisControl::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGroupBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: windowChanged(); break;
        case 1: changeLoEdge((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: changeHiEdge((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: auto_scale((*reinterpret_cast< bool(*)>(_a[1]))); break;
        }
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::AxisControl::windowChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
