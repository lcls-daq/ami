#include "EnvPlot.hh"

#include "ami/qt/AxisArray.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtTH2F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtProf2D.hh"
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
#include "ami/data/EntryProf2D.hh"
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
  EnvOp    (filter, desc, set),
  _plot    (new QtEmpty),
  _auto_range(0),
  _retry     (false),
  _shared  (shared)
{
  if (shared) shared->signup();
  _plot->attach(_frame);

  setPlotType(_desc->type());
}

EnvPlot::EnvPlot(QWidget*         parent,
		 const QString&   name,
		 const Ami::AbsFilter&  filter,
		 DescEntry*       desc,
		 AbsOperator*     op,
                 unsigned         channel,
                 SharedData*      shared) :
  QtPlot   (parent, name),
  EnvOp    (filter, desc, op, channel),
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
  _plot    (new QtEmpty),
  _auto_range(0),
  _shared  (0)
{
  XML_iterate_open(p,tag)
    
    if (tag.element == "QtPlot")
      QtPlot::load(p);
    else if (EnvOp::load(tag,p))
      ;
  XML_iterate_close(EnvPlot,tag);

  _plot->attach(_frame);
  setPlotType(_desc->type());
}

EnvPlot::~EnvPlot()
{
  { QtBase* plot = _plot;
    _plot = 0;
    delete plot; }
  if (_shared  ) _shared->resign();
}

void EnvPlot::save(char*& p) const
{
  XML_insert( p, "QtPlot", "self", QtPlot::save(p) );
  EnvOp::save(p);
}


void EnvPlot::load(const char*& p)
{
}

void EnvPlot::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"

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

#define CASE_ENTRY(t) \
      case Ami::DescEntry::t:                                         \
        _plot = new Qt##t(_name,*static_cast<const Ami::Entry##t*>(entry), \
                          Ami::AbsTransform::null(),                    \
                          Ami::AbsTransform::null(),                    \
                          QColor(0,0,0));                               \
      break;

      switch(entry->desc().type()) {
        CASE_ENTRY(TH1F)
        CASE_ENTRY(TH2F)
        CASE_ENTRY(Prof)
        CASE_ENTRY(Prof2D)
      case Ami::DescEntry::ScalarRange: 
        _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::ScalarDRange: 
        _auto_range = static_cast<const Ami::EntryScalarDRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        _plot = new QtChart(_name,*static_cast<const Ami::EntryScalar*>(entry),
                            QColor(0,0,0));
        edit_xrange(false);
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(_name,*static_cast<const Ami::EntryScan*>(entry),
                           Ami::AbsTransform::null(),
                           Ami::AbsTransform::null(),
                           QColor(0,0,0),
                           _style.symbol_size(),_style.symbol_style());
        break;
      default:
        printf("EnvPlot type %d not implemented yet\n",entry->desc().type()); 
        _plot = new QtEmpty;
      }
      _plot->attach(_frame);
      emit curve_changed();
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

bool EnvPlot::_forceRequest() const { return _plot==0; }

void EnvPlot::update()
{
  if (_plot) {
    _plot->update();
    update_fit(_plot->entry());
    emit counts_changed(_plot->normalization());
    updated();
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
