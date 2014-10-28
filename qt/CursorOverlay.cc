#include "CursorOverlay.hh"

#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtPlot.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/Cds.hh"
#include "ami/data/AbsTransform.hh"
#include "ami/data/DescEntry.hh"

#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"

#include <QtGui/QLabel>
#include "qwt_plot.h"

using namespace Ami::Qt;

CursorOverlay::CursorOverlay(OverlayParent&   parent,
                             QtPlot&          frame,
                             unsigned         channel,
                             BinMath*         input) :
  QtOverlay  (parent),
  _frame     (&frame),
  _frame_name(frame._name),
  _plot      (0),
  _auto_range(0),
  _order     (-1)
{
  _channel   = channel;
  _input     = input;
  _output_signature = 0;
}

CursorOverlay::CursorOverlay(OverlayParent& parent,
                             const char*&   p) :
  QtOverlay  (parent),
  _frame     (0),
  _plot      (0),
  _auto_range(0),
  _order     (-1)
{
  _output_signature = 0;
  load(p);
}

CursorOverlay::~CursorOverlay()
{
  delete _input;
  if (_plot  ) delete _plot;
}

void CursorOverlay::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "QString"  , "_frame_name", QtPersistent::insert(p, _frame_name) );
  XML_insert( p, "int", "_channel", QtPersistent::insert(p,(int)_channel) );
  XML_insert( p, "BinMath", "_input", QtPersistent::insert(p,buff,(char*)_input->serialize(buff)-buff) );
  delete[] buff;
}

void CursorOverlay::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_frame_name")
      _frame_name = QtPersistent::extract_s(p);
    else if (tag.name == "_channel")
      _channel = QtPersistent::extract_i(p);
    else if (tag.name == "_input") {
      const char* b = (const char*)QtPersistent::extract_op(p);
      b += 2*sizeof(uint32_t);
      _input = new BinMath(b);
    }
  XML_iterate_close(CursorOverlay,tag);

  _output_signature=0;
}

void CursorOverlay::dump(FILE* f) const { _plot->dump(f); }

#include "ami/data/Entry.hh"

void CursorOverlay::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry) {

    if (_plot && !_req.changed() && !_auto_range) {
      _plot->entry(*entry);
    }
    else {
      if (_plot)
        delete _plot;
    
      QColor c(0,0,0);
      _auto_range = 0;

      QString name(_input->output().name());
      switch(entry->desc().type()) {
      case Ami::DescEntry::TH1F: 
        _plot = new QtTH1F(name,*static_cast<const Ami::EntryTH1F*>(entry),
                           Ami::AbsTransform::null(),Ami::AbsTransform::null(),c);
        break;
      case Ami::DescEntry::Scalar:  // create a chart from a scalar
        _plot = new QtChart(name,*static_cast<const Ami::EntryScalar*>(entry),c);
        break;
      case Ami::DescEntry::ScalarRange:
        _auto_range = static_cast<const Ami::EntryScalarRange*>(entry);
        _plot = 0;
        return;
      case Ami::DescEntry::Prof: 
        _plot = new QtProf(name,*static_cast<const Ami::EntryProf*>(entry),
                           Ami::AbsTransform::null(),Ami::AbsTransform::null(),c);
        break;
      case Ami::DescEntry::Scan: 
        _plot = new QtScan(name,*static_cast<const Ami::EntryScan*>(entry),
                           Ami::AbsTransform::null(),Ami::AbsTransform::null(),c);
        break;
      default:
        printf("CursorOverlay type %d not implemented yet\n",entry->desc().type()); 
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
      printf("%s output_signature %d not found\n",_input->output().name(),_output_signature);
    if (_plot) {
      delete _plot;
      _plot = 0;
    }
  }
}

void CursorOverlay::update()
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
      _auto_range->result(&_input->output());
      _auto_range = 0;
      emit changed();
    }
  }
}

void CursorOverlay::_attach()
{
  if (_order<0) {
    _order = _frame->_frame->itemList().size();
    attach(*_frame);
  }

  _plot->set_color(_order-1);
  _plot->attach(_frame->_frame);
  _frame->set_style();
}

const QtBase* CursorOverlay::base() const { return _plot; }
