#include "EdgeCursor.hh"

#include "ami/qt/PlotFrame.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtCore/QString>

#include "qwt_plot_marker.h"

using namespace Ami::Qt;

EdgeCursor::EdgeCursor(const QString& name,
		       PlotFrame&     frame) :
  QWidget(0),
  _frame (frame),
  _input (new QLineEdit()),
  _marker(new QwtPlotMarker)
{
  _marker->setLabel(name);
  _marker->setLineStyle(QwtPlotMarker::HLine);

  QPushButton* grabB = new QPushButton("Grab");
  _showB = new QPushButton("Show"); 
  _showB->setCheckable(true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(name));
  layout->addWidget(_input);
  layout->addWidget(grabB);
  layout->addWidget(_showB);
  setLayout(layout);

  connect(_input, SIGNAL(editingFinished()), this, SLOT(set_value()));
  connect(grabB , SIGNAL(clicked())        , this, SLOT(grab()));
  connect(_showB, SIGNAL(clicked(bool))    , this, SLOT(show_in_plot(bool)));
  connect(this  , SIGNAL(changed())        , &frame, SLOT(replot()));
}

EdgeCursor::~EdgeCursor()
{
  delete _marker;
}

double EdgeCursor::value() const { return _input->text().toDouble(); }
void   EdgeCursor::value(double v) { _input->setText(QString::number(v)); set_value(); }

void EdgeCursor::set_value()
{
  _marker->setYValue(value());
  _showB->setChecked(true);
  show_in_plot(true);
  emit changed();
}

void EdgeCursor::grab()
{
  _frame.set_cursor_input(this);
}

void EdgeCursor::show_in_plot(bool lShow)
{
  if (lShow) _marker->attach(static_cast<PlotFrame*>(&_frame));
  else       _marker->attach(NULL);
  emit changed();
}

void EdgeCursor::mousePressEvent(double x, double y)
{
  _frame.set_cursor_input(0);
  _input->setText(QString::number(y));
  set_value();
}

void EdgeCursor::mouseReleaseEvent(double x, double y) {}
