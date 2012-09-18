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
  _frame   (&frame),
  _frame_name(frame._name),
  _filter  (filter.clone()),
  _desc    (desc),
  _set     (set),
  _output_signature  (0),
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
  _filter  (0),
  _set     (Ami::PreAnalysis),
  _output_signature(0),
  _plot    (0),
  _auto_range(0),
  _order   (-1),
  _shared  (0)
{
  XML_iterate_open(p,tag)
    
    if (tag.name == "_frame_name")
      _frame_name = QtPersistent::extract_s(p);
    else if (tag.name == "_filter") {
      Ami::FilterFactory factory;
      const char* b = (const char*)QtPersistent::extract_op(p);
      _filter = factory.deserialize(b);
    }
    else if (tag.name == "_desc") {
      DescEntry* desc = (DescEntry*)QtPersistent::extract_op(p);

#define CASEENTRY(type) case DescEntry::type: _desc = new Desc##type(*static_cast<Desc##type*>(desc)); break;

      switch(desc->type()) {
        CASEENTRY(TH1F)
          CASEENTRY(Prof)
          CASEENTRY(Scan)
          CASEENTRY(Scalar)
          default: break;
      }
    }
    else if (tag.name == "_set") {
      _set = Ami::ScalarSet(QtPersistent::extract_i(p));
    }

  XML_iterate_close(EnvOverlay,tag);
}

EnvOverlay::~EnvOverlay()
{
  if (_shared) _shared->resign();
  if (_plot  ) delete _plot;
  if (_filter) delete _filter;
  delete _desc;
}

void EnvOverlay::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "QString"  , "_frame_name", QtPersistent::insert(p, _frame_name) );
  XML_insert( p, "AbsFilter", "_filter", QtPersistent::insert(p, buff, (char*)_filter->serialize(buff)-buff) );
  XML_insert( p, "DescEntry", "_desc", QtPersistent::insert(p, _desc, _desc->size()) );
  XML_insert( p, "ScalarSet", "_set" , QtPersistent::insert(p, int(_set)) );
  delete[] buff;
}


void EnvOverlay::load(const char*& p)
{
}

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EnvOverlay::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    
    if (_plot && !_req.changed() && !_auto_range) {
      _plot->entry(*entry);
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
        _attach();
    }

    if (!_frame && (_frame = QtPlot::lookup(_frame_name)))
      _attach();
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

void EnvOverlay::configure(char*& p, unsigned input, unsigned& output)
{
  Ami::EnvPlot op(*_desc);
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Discovery,
						  input,
						  -1,
						  *_filter, op, _set);
  p += r.size();
  _req.request(r, output, _plot==0);
  _output_signature = r.output();
}

void EnvOverlay::update()
{
  if (_plot) {
    //  This may be unnecessary
    if (!_frame && (_frame = QtPlot::lookup(_frame_name))) {
      printf("Late EnvOverlay attach to %s\n",qPrintable(_frame_name));
      _attach();
    }

    _plot->update();
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

void EnvOverlay::_attach()
{
  if (_order<0) {
    _order = _frame->_frame->itemList().size();
    attach(*_frame);
  }

  _plot->set_color(_order-1);
  _plot->attach(_frame->_frame);
  _frame->set_style();
}
