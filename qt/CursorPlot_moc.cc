/****************************************************************************
** Meta object code from reading C++ file 'CursorPlot.hh'
**
** Created: Fri Sep 4 19:02:20 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "CursorPlot.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CursorPlot.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__CursorPlot[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      21,   20,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
      30,   20,   20,   20, 0x0a,
      42,   20,   20,   20, 0x0a,
      59,   20,   20,   20, 0x0a,
      77,   20,   20,   20, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__CursorPlot[] = {
    "Ami::Qt::CursorPlot\0\0redraw()\0save_data()\0"
    "set_plot_title()\0set_xaxis_title()\0"
    "set_yaxis_title()\0"
};

const QMetaObject Ami::Qt::CursorPlot::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__CursorPlot,
      qt_meta_data_Ami__Qt__CursorPlot, 0 }
};

const QMetaObject *Ami::Qt::CursorPlot::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::CursorPlot::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__CursorPlot))
	return static_cast<void*>(const_cast< CursorPlot*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::CursorPlot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: redraw(); break;
        case 1: save_data(); break;
        case 2: set_plot_title(); break;
        case 3: set_xaxis_title(); break;
        case 4: set_yaxis_title(); break;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void Ami::Qt::CursorPlot::redraw()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
