#include "EnvOverlay.hh"

#include "ami/qt/OverlayParent.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtPlot.hh"
#include "ami/qt/QtPlotStyle.hh"
#include "ami/qt/SharedData.hh"

#include "ami/data/AbsFilter.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EnvPlot.hh"

#include "qwt_plot.h"

using namespace Ami::Qt;

EnvOverlay::EnvOverlay(OverlayParent&   parent,
                       QtPlot&          frame,
                       const Ami::AbsFilter&  filter,
                       DescEntry*       desc,
                       Ami::ScalarSet   set,
                       SharedData*      shared) :
  QtOverlay(parent),
  EnvOp    (filter, desc, set),
  _frame   (&frame),
  _frame_name(frame._name),
  _plot    (0),
  _auto_range(0),
  _order   (-1),
  _shared  (shared)
{
  if (shared) shared->signup();
}

EnvOverlay::EnvOverlay(OverlayParent&   parent,
                       QtPlot&          frame,
                       const Ami::AbsFilter&  filter,
                       DescEntry*       desc,
                       AbsOperator*     op,
                       unsigned         channel,
                       SharedData*      shared) :
  QtOverlay(parent),
  EnvOp    (filter, desc, op, channel),
  _frame   (&frame),
  _frame_name(frame._name),
  _plot    (0),
  _auto_range(0),
  _order   (-1),
  _shared  (shared)
{
  if (shared) shared->signup();
}

EnvOverlay::EnvOverlay(OverlayParent& parent,
                       const char*& p) :
  QtOverlay(parent),
  _frame   (0),
  _plot    (0),
  _auto_range(0),
  _order   (-1),
  _shared  (0)
{
  XML_iterate_open(p,tag)
    
    if (tag.name == "_frame_name")
      _frame_name = QtPersistent::extract_s(p);
    else if (EnvOp::load(tag,p))
      ;

  XML_iterate_close(EnvOverlay,tag);
}

EnvOverlay::~EnvOverlay()
{
  if (_shared) _shared->resign();
  if (_plot  ) delete _plot;
}

void EnvOverlay::save(char*& p) const
{
  XML_insert( p, "QString"  , "_frame_name", QtPersistent::insert(p, _frame_name) );
  EnvOp::save(p);
}


void EnvOverlay::load(const char*& p) {}

void EnvOverlay::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"

void EnvOverlay::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    
    if (_plot && !_req.changed() && !_auto_range) {
      _plot->entry(*entry);
      if (!_frame && (_frame = QtPlot::lookup(_frame_name)))
        _attach(cds);
      else if (_frame)
        attach(*_frame,cds);
    }

    else {
      if (_plot)
        delete _plot;

      //      edit_xrange(true);
      QColor c(0,0,0);
      _auto_range = 0;
      
      switch(entry->desc().type()) {
      case Ami::DescEntry::TH1F: 
        _plot = new QtTH1F(_desc->name(),
                           *static_cast<const Ami::EntryTH1F*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c);
        break;
      case Ami::DescEntry::ScalarRange: 
        _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        //       { const DescChart& d = *reinterpret_cast<const DescChart*>(_desc);
        // 	_plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
        // 			    d.pts(),QColor(0,0,0));
        // 	break; }
        _plot = new QtChart(_desc->name(),
                            *static_cast<const Ami::EntryScalar*>(entry),
                            c);
        //        edit_xrange(false);
        break;
      case Ami::DescEntry::Prof: 
        _plot = new QtProf(_desc->name(),
                           *static_cast<const Ami::EntryProf*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c);
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(_desc->name(),
                           *static_cast<const Ami::EntryScan*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           c,
                           QtPlotStyle().symbol_size(),QtPlotStyle().symbol_style());
        break;
      default:
        printf("EnvOverlay type %d not implemented yet\n",entry->desc().type()); 
        _plot = 0;
        return;
      }

      if (_frame)
        _attach(cds);
    }

    if (!_frame && (_frame = QtPlot::lookup(_frame_name)))
      _attach(cds);
  }
  else {
    if (_output_signature>=0)
      printf("%s output_signature %d not found\n",_desc->name(),_output_signature);
    if (_plot) {
      delete _plot;
      _plot = 0;
    }
  }
}

bool EnvOverlay::_forceRequest() const { return _plot==0; }

void EnvOverlay::update()
{
  if (_plot) {
    _plot->update();
    emit updated();
  }

  if (_auto_range) {
    double v = _auto_range->entries() - double(_auto_range->desc().nsamples());
    if (v >= 0) {
      delete _desc;
      _desc = _auto_range->result();
      _auto_range = 0;
      emit changed();
    }
  }
}

void EnvOverlay::_attach(Cds& cds)
{
  if (_order<0) {
    _order = _frame->_frame->itemList().size();
  }

  _plot->set_color(_order-1);
  _plot->attach(_frame->_frame);
  _frame->set_style();
  attach(*_frame,cds);
}

const QtBase* EnvOverlay::base() const { return _plot; }
