/****************************************************************************
** Meta object code from reading C++ file 'ChannelMath.hh'
**
** Created: Wed Sep 16 12:31:06 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "ChannelMath.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChannelMath.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__ChannelMath[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__ChannelMath[] = {
    "Ami::Qt::ChannelMath\0\0calc()\0"
};

const QMetaObject Ami::Qt::ChannelMath::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__ChannelMath,
      qt_meta_data_Ami__Qt__ChannelMath, 0 }
};

const QMetaObject *Ami::Qt::ChannelMath::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::ChannelMath::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__ChannelMath))
	return static_cast<void*>(const_cast< ChannelMath*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::ChannelMath::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: calc(); break;
        }
        _id -= 1;
    }
    return _id;
}
