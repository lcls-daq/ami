/****************************************************************************
** Meta object code from reading C++ file 'ChannelDefinition.hh'
**
** Created: Fri Sep 4 12:47:28 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "ChannelDefinition.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChannelDefinition.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__ChannelDefinition[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      28,   27,   27,   27, 0x05,
      51,   27,   27,   27, 0x05,

 // slots: signature, parameters, type, tag, flags
      61,   27,   27,   27, 0x0a,
      78,   27,   27,   27, 0x0a,
      94,   27,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__ChannelDefinition[] = {
    "Ami::Qt::ChannelDefinition\0\0"
    "reference_loaded(bool)\0changed()\0"
    "load_reference()\0show_plot(bool)\0"
    "apply()\0"
};

const QMetaObject Ami::Qt::ChannelDefinition::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__ChannelDefinition,
      qt_meta_data_Ami__Qt__ChannelDefinition, 0 }
};

const QMetaObject *Ami::Qt::ChannelDefinition::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::ChannelDefinition::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__ChannelDefinition))
	return static_cast<void*>(const_cast< ChannelDefinition*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::ChannelDefinition::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: reference_loaded((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: changed(); break;
        case 2: load_reference(); break;
        case 3: show_plot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: apply(); break;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::ChannelDefinition::reference_loaded(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Ami::Qt::ChannelDefinition::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
