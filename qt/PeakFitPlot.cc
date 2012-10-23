#include "PeakFitPlot.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtTH2F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtEmpty.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/FeatureRegistry.hh"

#include "ami/data/PeakFitPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

namespace Ami {
  namespace Qt {
    class NullTransform : public Ami::AbsTransform {
    public:
      ~NullTransform() {}
      double operator()(double x) const { return x; }
    };
  };
};

using namespace Ami::Qt;

static NullTransform noTransform;

PeakFitPlot::PeakFitPlot(QWidget* parent,
			 const QString&    name,
			 unsigned          channel,
			 Ami::PeakFitPlot* input) :
  QtPlot   (parent, name),
  _channel (channel),
  _input   (input),
  _output_signature  (0),
  _plot    (new QtEmpty),
  _auto_range(0)
{
  _plot->attach(_frame);
  setPlotType(input->output().type());
}

PeakFitPlot::PeakFitPlot(QWidget* parent,
			 const char*& p) :
  QtPlot(parent),
  _auto_range(0)

{
  load(p);

  _output_signature=0;
  _plot = new QtEmpty;
  _plot->attach(_frame);

  setPlotType(_input->output().type());
}

PeakFitPlot::~PeakFitPlot()
{
  delete _input;
  if (_plot    ) delete _plot;
}

void PeakFitPlot::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert(p, "QtPlot", "self", QtPlot::save(p) );
  XML_insert(p, "unsigned", "_channel", QtPersistent::insert(p,(int)_channel) );
  XML_insert(p, "PeakFitPlot", "_input", QtPersistent::insert(p,buff,(char*)_input->serialize(buff)-buff) );
  delete[] buff;
}

void PeakFitPlot::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "QtPlot")
      QtPlot::load(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_input") {
      const char* b = (const char*)QtPersistent::extract_op(p);
      b += 2*sizeof(uint32_t);
      _input = new Ami::PeakFitPlot(b);
    }
  XML_iterate_close(PeakFitPlot,tag);
}

void PeakFitPlot::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void PeakFitPlot::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    if (_plot && !_req.changed() && !_auto_range) {
      _plot->entry(*entry);
    }
    else {
      if (_plot)
        delete _plot;

      _auto_range = 0;

      edit_xrange(true);
      edit_yrange(true);

      switch(entry->desc().type()) {
      case Ami::DescEntry::TH1F: 
        _plot = new QtTH1F(_name,*static_cast<const Ami::EntryTH1F*>(entry),
                           noTransform,noTransform,QColor(0,0,0));
        break;
      case Ami::DescEntry::TH2F: 
        _plot = new QtTH2F(_name,*static_cast<const Ami::EntryTH2F*>(entry),
                           noTransform,noTransform,QColor(0,0,0));
        break;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        _plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
                            QColor(0,0,0));
        edit_xrange(false);
        break;
      case Ami::DescEntry::Prof: 
        _plot = new QtProf(_name,*static_cast<const Ami::EntryProf*>(entry),
                           noTransform,noTransform,QColor(0,0,0));
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(_name,*static_cast<const Ami::EntryScan*>(entry),
                           noTransform,noTransform,QColor(0,0,0),
                           _style.symbol_size());
        break;
      case Ami::DescEntry::ScalarRange:
        _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
        _plot = new QtEmpty;
        return;
      case Ami::DescEntry::ScalarDRange:
        _auto_range = static_cast<const Ami::EntryScalarDRange*>(entry);
        _plot = new QtEmpty;
        return;
      default:
        printf("PeakFitPlot type %d not implemented yet\n",entry->desc().type()); 
        _plot = new QtEmpty;
      }
      _plot->attach(_frame);
    }
  }
  else {
    if (_output_signature>=0)
      printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
    if (_plot) {
      delete _plot;
      _plot = new QtEmpty;
    }
  }
}

void PeakFitPlot::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input_signature,
						  -1,
						  *channels[channel]->filter().filter(),
						  *_input);
  p += r.size();
  _req.request(r,output);
  _output_signature = r.output();
}

void PeakFitPlot::update()
{
  if (_plot) {
    _plot->update();
    emit counts_changed(_plot->normalization());
    emit redraw();
  }
  if (_auto_range) {
    double v = _auto_range->entries();
    emit counts_changed(v);
    if (v >= 0) {
      _auto_range->result(&_input->output());
      _auto_range = 0;
      emit changed();
    }
  }
}
