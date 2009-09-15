/****************************************************************************
** Meta object code from reading C++ file 'FeatureRegistry.hh'
**
** Created: Wed Sep 9 19:26:10 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "FeatureRegistry.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FeatureRegistry.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__FeatureRegistry[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      26,   25,   25,   25, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__FeatureRegistry[] = {
    "Ami::Qt::FeatureRegistry\0\0changed()\0"
};

const QMetaObject Ami::Qt::FeatureRegistry::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Ami__Qt__FeatureRegistry,
      qt_meta_data_Ami__Qt__FeatureRegistry, 0 }
};

const QMetaObject *Ami::Qt::FeatureRegistry::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::FeatureRegistry::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__FeatureRegistry))
	return static_cast<void*>(const_cast< FeatureRegistry*>(this));
    return QObject::qt_metacast(_clname);
}

int Ami::Qt::FeatureRegistry::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: changed(); break;
        }
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::FeatureRegistry::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
