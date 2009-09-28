#include "ZoomPlot.hh"

#include "ami/qt/QtImage.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageFrame.hh"

#include "ami/data/Cds.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>

#include "qwt_plot.h"

using namespace Ami::Qt;

ZoomPlot::ZoomPlot(QWidget*         parent,
		   const QString&   name,
		   unsigned         input_channel,
		   unsigned         x0, 
		   unsigned         y0,
		   unsigned         x1,
		   unsigned         y1) :
  QWidget  (parent,::Qt::Window),
  _name    (name),
  _input   (input_channel),
  _signature(-1),
  _x0     (x0),
  _y0     (y0),
  _x1     (x1),
  _y1     (y1),
  _frame   (new ImageDisplay)
{
  _frame->plot()->autoXYScale(true);

  setWindowTitle(name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();
}

ZoomPlot::~ZoomPlot()
{
}

void ZoomPlot::setup_payload(Cds& cds)
{
  _frame->reset();
  const Entry* entry = cds.entry(_signature);
  _frame->add( new QtImage(entry->desc().name(),
			   *static_cast<const EntryImage*>(entry),
			   _x0, _y0, _x1, _y1) );
}

void ZoomPlot::configure(char*& p, unsigned input, unsigned& output,
			  ChannelDefinition* input_channels[], int* input_signatures, unsigned input_nchannels)
{
  _signature = input_signatures[_input];
}

void ZoomPlot::update()
{
  _frame  ->update();
}
