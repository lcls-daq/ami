#include "CursorDefinition.hh"
#include "CursorsX.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include "qwt_plot_marker.h"

using namespace Ami::Qt;

CursorDefinition::CursorDefinition(const QString& name,
				   double    location,
				   CursorsX& parent,
				   QwtPlot*  plot) :
  QWidget  (0),
  _name    (name),
  _location(location),
  _parent  (parent),
  _plot    (plot)
{
  _marker = new QwtPlotMarker;
  _marker->setLabel    (name);
  _marker->setLineStyle(QwtPlotMarker::VLine);
  _marker->setXValue   (location);
  
  QPushButton* showB = new QPushButton("Show"); showB->setCheckable(true);
  QPushButton* delB  = new QPushButton("Delete");

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(QString("%1 @ %2").arg(name).arg(location)));
  layout->addStretch();
  layout->addWidget(showB);
  layout->addWidget(delB);
  setLayout(layout);

  connect(delB, SIGNAL(clicked()), this, SLOT(remove()));
  connect(showB, SIGNAL(clicked(bool)), this, SLOT(show_in_plot(bool)));

  showB->click();
}

CursorDefinition::~CursorDefinition()
{
  delete _marker;
}

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
