/****************************************************************************
** Meta object code from reading C++ file 'DescTH1F.hh'
**
** Created: Tue Sep 1 14:32:38 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "DescTH1F.hh"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DescTH1F.hh' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_Ami__Qt__DescTH1F[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets

       0        // eod
};

static const char qt_meta_stringdata_Ami__Qt__DescTH1F[] = {
    "Ami::Qt::DescTH1F\0"
};

const QMetaObject Ami::Qt::DescTH1F::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Ami__Qt__DescTH1F,
      qt_meta_data_Ami__Qt__DescTH1F, 0 }
};

const QMetaObject *Ami::Qt::DescTH1F::metaObject() const
{
    return &staticMetaObject;
}

void *Ami::Qt::DescTH1F::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Ami__Qt__DescTH1F))
	return static_cast<void*>(const_cast< DescTH1F*>(this));
    return QWidget::qt_metacast(_clname);
}

int Ami::Qt::DescTH1F::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
