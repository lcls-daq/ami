#include "Cursor.hh"

#include "ami/qt/PlotFrame.hh"
#include "ami/data/QtPersistent.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <QtCore/QString>

#include "qwt_plot_marker.h"

#include <stdio.h>

using namespace Ami::Qt;

Cursor::Cursor(Measure        measure,
               const QString& name,
               PlotFrame&     frame,
               QtPWidget*     frameParent) :
  QWidget(0),
  _measure(measure),
  _v     (0),
  _frame (frame),
  _frameParent(frameParent),
  _input (new QLineEdit()),
  _marker(new QwtPlotMarker)
{
  new QDoubleValidator(_input);

  _marker->setLabel(name);
  _marker->setLineStyle(_measure==Horizontal ? QwtPlotMarker::VLine:QwtPlotMarker::HLine);

  QPushButton* grabB = new QPushButton("Grab");
  _showB = new QPushButton("Show"); 
  _showB->setCheckable(true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addStretch();
  if (qPrintable(name)[0])
      layout->addWidget(new QLabel(name));
  layout->addWidget(_input);
  layout->addWidget(grabB);
  layout->addWidget(_showB);
  setLayout(layout);

  connect(_input, SIGNAL(editingFinished()), this, SLOT(set_value()));
  connect(grabB , SIGNAL(clicked())        , this, SLOT(grab()));
  connect(_showB, SIGNAL(toggled(bool))    , this, SLOT(show_in_plot(bool)));
  connect(this  , SIGNAL(replot())         , &frame, SLOT(replot()));
}

Cursor::~Cursor()
{
  delete _marker;
}

void Cursor::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "Cursors")
      value(QtPersistent::extract_d(p));
    else if (tag.name == "_showB")
      _showB->setChecked(QtPersistent::extract_b(p));
  XML_iterate_close(Cursor,tag);
}

void Cursor::save(char*& p) const 
{
  XML_insert(p, "Cursors", "self", QtPersistent::insert(p,value()) );
  XML_insert(p, "QPushButton", "_showB", QtPersistent::insert(p,_showB->isChecked()) );
}

double Cursor::value() const { return _v; }

void   Cursor::value(double v) 
{ 
  _input->setText(QString::number(v)); 
  set_value(); 
}

void Cursor::set_value()
{
  double v = _input->text().toDouble();

  if (_measure==Horizontal)
    _marker->setXValue(v);
  else
    _marker->setYValue(v);

  if (_v != v) {
    _v = v;
    _showB->setChecked(true);
    emit changed();
  }
}

void Cursor::grab()
{
  _frame.set_cursor_input(this);
  if (_frameParent)
    _frameParent->front();
}

void Cursor::show_in_plot(bool lShow)
{
  if (lShow) _marker->attach(static_cast<PlotFrame*>(&_frame));
  else       _marker->attach(NULL);
  emit replot();
}

void Cursor::mousePressEvent(double x, double y)
{
  _frame.set_cursor_input(0);
  _input->setText(QString::number(_measure==Horizontal ? x:y));
  set_value();
}

void Cursor::mouseMoveEvent   (double x, double y) {}
void Cursor::mouseReleaseEvent(double x, double y) {}

void Cursor::setName(QString &name) { _marker->setLabel(name); }
