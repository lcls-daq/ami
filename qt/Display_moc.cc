/****************************************************************************
** Meta object code from reading C++ file 'Display.hh'
**
** Created: Fri Sep 11 10:53:42 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Display.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Display.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Display[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
      27,   17,   17,   17, 0x0a,
      40,   17,   17,   17, 0x0a,
      52,   17,   17,   17, 0x0a,
      69,   17,   17,   17, 0x0a,
      78,   17,   17,   17, 0x0a,
      98,   17,   17,   17, 0x0a,
     114,   17,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Display[] = {
    "Ami::Qt::Display\0\0redraw()\0save_image()\0"
    "save_data()\0save_reference()\0update()\0"
    "xtransform_update()\0xrange_change()\0"
    "yrange_change()\0"
};

const QMetaObject Ami::Qt::Display::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__Display,
      qt_meta_data_Ami__Qt__Display, 0 }
};

const QMetaObject *Ami::Qt::Display::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Display::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Display))
	return static_cast<void*>(const_cast< Display*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::Display::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: redraw(); break;
        case 1: save_image(); break;
        case 2: save_data(); break;
        case 3: save_reference(); break;
        case 4: update(); break;
        case 5: xtransform_update(); break;
        case 6: xrange_change(); break;
        case 7: yrange_change(); break;
        }
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::Display::redraw()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
