#include "PeakPlot.hh"

#include "ami/qt/QtImage.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/NullTransform.hh"

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

static NullTransform _noTransform;
static QColor _noColor;

PeakPlot::PeakPlot(QWidget*         parent,
		   const QString&   name,
		   unsigned         input_channel,
		   unsigned         threshold) :
  QtPWidget(parent),
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

PeakPlot::PeakPlot(QWidget*         parent,
		   const char*&     p) :
  QtPWidget (parent),
  _signature(-1),
  _frame    (new ImageDisplay)
{
  load(p);

  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();
  connect(this, SIGNAL(redraw()), _frame, SLOT(replot()));

  QtPWidget::load(p);
}

PeakPlot::~PeakPlot()
{
}

void PeakPlot::save(char*& p) const
{
  XML_insert( p, "QString", "_name",
              QtPersistent::insert(p,_name) );
  XML_insert( p, "unsigned", "_input",
              QtPersistent::insert(p,_input) );
  XML_insert( p, "unsigned", "_threshold",
              QtPersistent::insert(p,_threshold) );
  XML_insert( p, "ImageDisplay", "_frame",
              _frame->save(p) );
  XML_insert( p, "QtPWidget", "self",
              QtPWidget::save(p) );
}

void PeakPlot::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.element == "QString")
      _name  = QtPersistent::extract_s(p);
    else if (tag.element == "unsigned") {
      if      (tag.name == "_input")
        _input = QtPersistent::extract_i(p);
      else if (tag.name == "_threshold")
        _threshold = QtPersistent::extract_i(p);
    }
    else if (tag.element == "ImageDisplay")
      _frame->load(p);
  XML_iterate_close(PeakPlot,tag);
}

void PeakPlot::setup_payload(Cds& cds)
{
  _frame->reset();
  const EntryImage* entry = static_cast<const EntryImage*>(cds.entry(_signature));
  const DescImage& d = entry->desc();
  _frame->add(new QtImage(d.name(), *entry, _noTransform, _noTransform, _noColor), true);
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
