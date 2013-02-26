#include "CurveFitOverlay.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtPlot.hh"

#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/CurveFit.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

using namespace Ami::Qt;

CurveFitOverlay::CurveFitOverlay(OverlayParent&   parent,
                                 QtPlot&          plot,
                                 unsigned         channel,
                                 Ami::CurveFit* fit) :
  QtOverlay(parent),
  _frame   (&plot),
  _frame_name(plot._name),
  _channel (channel),
  _fit     (fit),
  _output_signature (0),
  _plot    (0),
  _order   (-1)
{
}

CurveFitOverlay::CurveFitOverlay(OverlayParent& parent,
                                 const char*& p) :
  QtOverlay(parent),
  _frame   (0),
  _fit     (0),
  _plot    (0),
  _order   (-1)
{
  load(p);
}

CurveFitOverlay::~CurveFitOverlay()
{
  delete _fit;
  if (_plot    ) delete _plot;
}

void CurveFitOverlay::save(char*& p) const
{
  char* buff = new char[8*1024];

  XML_insert( p, "QString", "_frame_name", QtPersistent::insert(p, _frame_name));

  XML_insert( p, "int", "_channel",
              QtPersistent::insert(p,(int)_channel) );

  XML_insert( p, "CurveFit", "_fit", savefit(p));

  delete[] buff;
}

void CurveFitOverlay::savefit(char*& p) const
{
    DescEntry &desc = _fit->output();
    XML_insert( p, "QString"     , "_name"  , QtPersistent::insert(p, QString(_fit->name())));
    XML_insert( p, "QString"     , "_norm"  , QtPersistent::insert(p, QString(_fit->norm())));
    XML_insert( p, "int"         , "_op"    , QtPersistent::insert(p, _fit->op()) );
    XML_insert( p, "DescEntry"   , "_output", QtPersistent::insert(p, &desc, desc.size()));
}

Ami::CurveFit *CurveFitOverlay::loadfit(const char*& p)
{
  QString name;
  QString norm;
  int op = 0;
  DescEntry *desc = NULL;

  name.clear();
  XML_iterate_open(p,tag)
      if (tag.name == "_name")
          name = QtPersistent::extract_s(p);
      else if (tag.name == "_norm")
          norm = QtPersistent::extract_s(p);
      else if (tag.name == "_op")
          op = Ami::Qt::QtPersistent::extract_i(p);
      else if (tag.name == "_output")
          desc = (DescEntry*)QtPersistent::extract_op(p);
  XML_iterate_close(CurveFit,tag);

  if (desc)
      return new Ami::CurveFit(qPrintable(name), op, *desc, qPrintable(norm));
  else
      return NULL;
}

void CurveFitOverlay::load(const char*& p) 
{
  XML_iterate_open(p,tag)
    if (tag.name == "_frame_name")
      _frame_name = QtPersistent::extract_s(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_fit") {
      _fit = loadfit(p);
    }
  XML_iterate_close(CurveFitOverlay,tag);
}

void CurveFitOverlay::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void CurveFitOverlay::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    if (_plot && !_req.changed()) {
      _plot->entry(*entry);
    }
    else {
      if (_plot)
        delete _plot;

      QColor c(0,0,0);
      QString name(_fit->output().name());

      switch(entry->desc().type()) {
      case Ami::DescEntry::TH1F: 
        _plot = new QtTH1F(name,*static_cast<const Ami::EntryTH1F*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c);
        break;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        _plot = new QtChart(name,*static_cast<const Ami::EntryScalar*>(entry),c);
        break;
      case Ami::DescEntry::Prof: 
        _plot = new QtProf(name,*static_cast<const Ami::EntryProf*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c);
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(name,*static_cast<const Ami::EntryScan*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c);
        break;
      default:
        printf("CurveFitOverlay type %d not implemented yet\n",entry->desc().type()); 
        _plot = 0;
        return;
      }
      if (_frame)
        _attach();
    }

    if (!_frame && (_frame = QtPlot::lookup(_frame_name)))
      _attach();
  }
  else {
    if (_output_signature>=0)
      printf("%s output_signature %d not found\n",_fit->output().name(),_output_signature);
    if (_plot) {
      delete _plot;
      _plot = 0;
    }
  }
}

void CurveFitOverlay::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo)
{
  unsigned input_signature = signatures[_channel];

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Analysis,
						  input_signature,
                                                  -1,
						  *channels[_channel]->filter().filter(),
						  *_fit);
  p += r.size();
  _req.request(r, output);
  _output_signature = ++output;
}

void CurveFitOverlay::update()
{
  if (_plot) {
    //  This may be unnecessary
    if (!_frame && (_frame = QtPlot::lookup(_frame_name))) {
      printf("Late EnvOverlay attach to %s\n",qPrintable(_frame_name));
      _attach();
    }

    _plot->update();
  }
}

void CurveFitOverlay::_attach()
{
  if (_order<0) {
    _order = _frame->_frame->itemList().size();
    attach(*_frame);
  }

  _plot->set_color(_order-1);
  _plot->attach(_frame->_frame);
  _frame->set_style();
}

const QtBase* CurveFitOverlay::base() const { return _plot; }
