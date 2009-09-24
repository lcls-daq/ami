#include "PeakPlot.hh"

#include "ami/qt/QtImage.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"

#include "ami/data/Cds.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/ConfigureRequest.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>

#include "qwt_plot.h"

using namespace Ami::Qt;

PeakPlot::PeakPlot(const QString&   name,
		   unsigned         input_channel,
		   unsigned         threshold) :
  QWidget  (0),
  _name    (name),
  _input   (input_channel),
  _threshold(threshold),
  _signature(-1),
  _frame   (new ImageDisplay)
{
  setWindowTitle(name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));
}

PeakPlot::~PeakPlot()
{
}

void PeakPlot::setup_payload(Cds& cds)
{
  _frame->reset();
  const EntryImage* entry = static_cast<const EntryImage*>(cds.entry(_signature));
  const DescImage& d = entry->desc();
  _frame->add(new QtImage(d.name(), *entry, 0, 0, d.nbinsx()-1, d.nbinsy()-1));
}

void PeakPlot::configure(char*& p, unsigned input, unsigned& output,
			 ChannelDefinition* input_channels[], int* input_signatures, unsigned input_nchannels)
{
  Ami::PeakFinder op(_threshold);

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Analysis,
						  input_signatures[_input],
						  _signature = ++output,
						  *input_channels[_input]->filter().filter(),
						  op);
  p += r.size();

}

void PeakPlot::update()
{
  _frame  ->update();
}
