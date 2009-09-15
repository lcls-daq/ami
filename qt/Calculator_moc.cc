/****************************************************************************
** Meta object code from reading C++ file 'Calculator.hh'
**
** Created: Fri Sep 11 14:18:06 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Calculator.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Calculator.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__Calculator[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      21,   20,   20,   20, 0x08,
      36,   20,   20,   20, 0x08,
      51,   20,   20,   20, 0x08,
      72,   20,   20,   20, 0x08,
      91,   20,   20,   20, 0x08,
      99,   20,   20,   20, 0x08,
     110,   20,   20,   20, 0x08,
     128,   20,   20,   20, 0x08,
     144,   20,   20,   20, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__Calculator[] = {
    "Ami::Qt::Calculator\0\0digitClicked()\0"
    "pointClicked()\0parenthesesClicked()\0"
    "backspaceClicked()\0clear()\0clearAll()\0"
    "variableClicked()\0varvarClicked()\0"
    "varconClicked()\0"
};

const QMetaObject Ami::Qt::Calculator::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_Ami__Qt__Calculator,
      qt_meta_data_Ami__Qt__Calculator, 0 }
};

const QMetaObject *Ami::Qt::Calculator::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::Calculator::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__Calculator))
	return static_cast<void*>(const_cast< Calculator*>(this));
    return QDialog::qt_metacast(_clname);
}

int Ami::Qt::Calculator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: digitClicked(); break;
        case 1: pointClicked(); break;
        case 2: parenthesesClicked(); break;
        case 3: backspaceClicked(); break;
        case 4: clear(); break;
        case 5: clearAll(); break;
        case 6: variableClicked(); break;
        case 7: varvarClicked(); break;
        case 8: varconClicked(); break;
        }
        _id -= 9;
    }
    return _id;
}
