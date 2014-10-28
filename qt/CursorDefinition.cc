#include "CursorDefinition.hh"
#include "CursorsX.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QValidator>

#include "qwt_plot_marker.h"

#include <stdio.h>

using namespace Ami::Qt;

CursorDefinition::CursorDefinition(const QString& name,
				   double    location,
				   Cursors& parent,
				   QwtPlot*  plot) :
  QWidget  (0),
  _name    (new QLineEdit(name)),
  _location(location),
  _parent  (parent),
  _plot    (plot)
{
  _layout();
}

CursorDefinition::CursorDefinition(const char*& p,
				   Cursors& parent,
				   QwtPlot*  plot) :
  QWidget  (0),
  _name    (new QLineEdit),
  _parent  (parent),
  _plot    (plot)
{
  load(p);
  _layout();
}

CursorDefinition::~CursorDefinition()
{
  delete _marker;
}

void CursorDefinition::_layout()
{
  _name ->setMaximumWidth(40);
  _value  = new QLineEdit(QString::number(_location));
  _value->setMaximumWidth(80);
  new QDoubleValidator(_value);

  _marker = new QwtPlotMarker;
  _marker->setLabel    (_name->text());
  _marker->setLineStyle(QwtPlotMarker::VLine);
  _marker->setXValue   (_location);
  
  QPushButton* showB = new QPushButton("Show"); showB->setCheckable(true);
  QPushButton* delB  = new QPushButton("Delete");

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_name);
  layout->addWidget(new QLabel("@"));
  layout->addWidget(_value);
  layout->addStretch();
  layout->addWidget(showB);
  layout->addWidget(delB);
  setLayout(layout);

  connect(_name , SIGNAL(editingFinished()), this, SLOT(update()));
  connect(_value, SIGNAL(editingFinished()), this, SLOT(update()));
  connect(delB, SIGNAL(clicked()), this, SLOT(remove()));
  connect(showB, SIGNAL(clicked(bool)), this, SLOT(show_in_plot(bool)));

  showB->click();
}

QString CursorDefinition::name() const { return _name->text(); }

void CursorDefinition::show_in_plot(bool lShow)
{
  if (lShow)
    _marker->attach(_plot);
  else
    _marker->attach(NULL);
}

void CursorDefinition::remove()
{
  _marker->attach(NULL);
  _parent.remove(*this);
}

void CursorDefinition::update()
{
  _location = _value->text().toDouble();
  _marker->setLabel (_name->text());
  _marker->setXValue(_location);
  emit changed();
}

void CursorDefinition::save(char*& p) const
{
  XML_insert(p, "QString", "_name", QtPersistent::insert(p, _name->text()) );
  XML_insert(p, "double", "_location", QtPersistent::insert(p, _location) );
}

void CursorDefinition::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_name")
      _name->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_location")
      _location = QtPersistent::extract_d(p);
  XML_iterate_close(CursorDefinition,tag);
}
