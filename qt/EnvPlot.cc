#include "EnvPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtTH2F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtEmpty.hh"
#include "ami/qt/Path.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/SharedData.hh"

#include "ami/data/AbsFilter.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EnvPlot.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

using namespace Ami::Qt;

EnvPlot::EnvPlot(QWidget*         parent,
		 const QString&   name,
		 const Ami::AbsFilter&  filter,
		 DescEntry*       desc,
                 Ami::ScalarSet   set,
                 SharedData*      shared) :
  QtPlot   (parent, name),
  _filter  (filter.clone()),
  _desc    (desc),
  _set     (set),
  _output_signature  (0),
  _plot    (new QtEmpty),
  _auto_range(0),
  _retry     (false),
  _shared  (shared)
{
  if (shared) shared->signup();
  _plot->attach(_frame);

  setPlotType(_desc->type());
}

EnvPlot::EnvPlot(QWidget*     parent,
		 const char*& p) :
  QtPlot   (parent),
  _filter  (0),
  _set     (Ami::PreAnalysis),
  _output_signature(0),
  _plot    (new QtEmpty),
  _auto_range(0),
  _shared  (0)
{
  XML_iterate_open(p,tag)
    
    if (tag.element == "QtPlot")
      QtPlot::load(p);
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
          CASEENTRY(ScalarDRange)
          CASEENTRY(TH2F)
          default: break;
      }
    }
    else if (tag.name == "_set") {
      _set = Ami::ScalarSet(QtPersistent::extract_i(p));
    }

  XML_iterate_close(EnvPlot,tag);

  _plot->attach(_frame);
  setPlotType(_desc->type());
}

EnvPlot::~EnvPlot()
{
  if (_filter  ) delete _filter;
  delete _desc;
  delete _plot;
  if (_shared  ) _shared->resign();
}

void EnvPlot::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "QtPlot", "self", QtPlot::save(p) );
  XML_insert( p, "AbsFilter", "_filter", QtPersistent::insert(p, buff, (char*)_filter->serialize(buff)-buff) );
  XML_insert( p, "DescEntry", "_desc", QtPersistent::insert(p, _desc, _desc->size()) );
  XML_insert( p, "ScalarSet", "_set" , QtPersistent::insert(p, int(_set)) );
  delete[] buff;
}


void EnvPlot::load(const char*& p)
{
}

void EnvPlot::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

void EnvPlot::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {
    
    if (_plot && !_req.changed() && !_auto_range && !_retry) {
      _plot->entry(*entry);
    }

    else {
      if (_plot)
        delete _plot;

      edit_xrange(true);
      edit_yrange(true);

      _auto_range = 0;

      switch(entry->desc().type()) {
      case Ami::DescEntry::TH1F: 
        _plot = new QtTH1F(_name,*static_cast<const Ami::EntryTH1F*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           QColor(0,0,0));
        break;
      case Ami::DescEntry::ScalarRange: 
        _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::ScalarDRange: 
        _auto_range = static_cast<const Ami::EntryScalarDRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        //       { const DescChart& d = *reinterpret_cast<const DescChart*>(_desc);
        // 	_plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
        // 			    d.pts(),QColor(0,0,0));
        // 	break; }
        _plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
                            QColor(0,0,0));
        edit_xrange(false);
        break;
      case Ami::DescEntry::Prof: 
        _plot = new QtProf(_name,*static_cast<const Ami::EntryProf*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           QColor(0,0,0));
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(_name,*static_cast<const Ami::EntryScan*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           QColor(0,0,0),
                           _style.symbol_size(),_style.symbol_style());
        break;
      case Ami::DescEntry::TH2F: 
        _plot = new QtTH2F(_name,*static_cast<const Ami::EntryTH2F*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           QColor(0,0,0));
        edit_xrange(false);
        edit_yrange(false);
        break;
      default:
        printf("EnvPlot type %d not implemented yet\n",entry->desc().type()); 
        _plot = new QtEmpty;
      }
      _plot->attach(_frame);
      _retry = false;
    }
  }
  else {
    if (_output_signature>=0)
      printf("%s output_signature %d not found\n",qPrintable(_name),_output_signature);
    if (_plot) {
      delete _plot;
      _plot = new QtEmpty;
      _retry = true;
    }
  }
}

void EnvPlot::configure(char*& p, unsigned input, unsigned& output)
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

void EnvPlot::update()
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
      delete _desc;
      _desc = _auto_range->result();
      _auto_range = 0;
      emit changed();
    }
  }
}
