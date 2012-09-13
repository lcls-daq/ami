#include "ami/qt/QtBase.hh"

#include <QtGui/QColor>

static QColor _colors[] = { QColor(  0,  0,  0),
                            QColor(255,  0,  0),
                            QColor(  0,129,  0),
                            QColor(  0,  0,255),
                            QColor(193,  0,193),
                            QColor(  0,193,193),
                            QColor(255,255,  0) };

static const unsigned NCOLORS = sizeof(_colors)/sizeof(QColor);


const QColor& Ami::Qt::QtBase::color(unsigned i)
{
  return _colors[ i%NCOLORS ];
}
