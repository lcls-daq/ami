/****************************************************************************
** Meta object code from reading C++ file 'Client.hh'
**
** Created: Sun Sep 13 17:46:47 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Client.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Client.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Client[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x05,

 // slots: signature, parameters, type, tag, flags
      42,   16,   16,   16, 0x0a,
      65,   16,   16,   16, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Client[] = {
    "Ami::Qt::Client\0\0description_changed(int)\0"
    "update_configuration()\0_read_description(int)\0"
};

const QMetaObject Ami::Qt::Client::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Client,
      qt_meta_data_Ami__Qt__Client, 0 }
};

const QMetaObject *Ami::Qt::Client::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Client::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Client))
	return static_cast<void*>(const_cast< Client*>(this));
    if (!strcmp(_clname, "Ami::AbsClient"))
	return static_cast< Ami::AbsClient*>(const_cast< Client*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Client::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: description_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: update_configuration(); break;
        case 2: _read_description((*reinterpret_cast< int(*)>(_a[1]))); break;
        }
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Client::description_changed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
