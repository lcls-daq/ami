#include "CursorOverlay.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PlotFactory.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtPlot.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
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
  _channel   (channel),
  _input     (input),
  _output_signature  (0),
  _plot      (0),
  _auto_range(0),
  _order     (-1)
{
}

CursorOverlay::CursorOverlay(OverlayParent& parent,
                             const char*&   p) :
  QtOverlay  (parent),
  _frame     (0),
  _output_signature(0),
  _plot      (0),
  _auto_range(0),
  _order     (-1)
{
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

#include "ami/data/Entry.hh"
#include "ami/data/DescEntry.hh"

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

void CursorOverlay::configure(char*& p, unsigned input, unsigned& output,
			   ChannelDefinition* channels[], int* signatures, unsigned nchannels,
			   const AxisInfo& xinfo, ConfigureRequest::Source source)
{
  unsigned channel = _channel;
  unsigned input_signature = signatures[channel];

  // replace cursor values with bin indices
  QString expr(_input->expression());
  QString new_expr;
  { QRegExp match("\\[[^\\]]*\\]");
    int last=0;
    int pos=0;
    while( (pos=match.indexIn(expr,pos)) != -1) {
      QString use = expr.mid(pos+1,match.matchedLength()-2);
      bool ok;
      double v = use.toDouble(&ok);
      unsigned bin=0;
      if (!ok)
	printf("error parsing double %s\n",qPrintable(use));
      else {
	bin = xinfo.tick(v);
      }
      new_expr.append(expr.mid(last,pos-last));
      new_expr.append(QString("[%1]").arg(bin));
      pos += match.matchedLength();
      last = pos;
    }
    new_expr.append(expr.mid(last));
    new_expr.replace(QString("]%1[").arg(BinMath::integrate()),QString(BinMath::integrate()));
    new_expr.replace(QString("]%1[").arg(BinMath::moment1  ()),QString(BinMath::moment1  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::moment2  ()),QString(BinMath::moment2  ()));
    new_expr.replace(QString("]%1[").arg(BinMath::range    ()),QString(BinMath::range    ()));
  }
  QString end_expr;
  { int last=0, next=0, pos=0;
    while( (pos=new_expr.indexOf(BinMath::range(),pos)) != -1) {
      if ( (next=new_expr.lastIndexOf("[",pos))==-1 )
	printf("error parsing range in %s\n",qPrintable(expr));
      else {
	end_expr.append(new_expr.mid(last,next-last));
	last  = new_expr.indexOf("]",pos);
	int a = new_expr.mid(next+1,pos -next-1).toInt();
	int b = new_expr.mid(pos +1,last-pos -1).toInt();
	printf("%s/%d %s/%d\n",
	       qPrintable(new_expr.mid(next+1,pos -next-1)),a,
	       qPrintable(new_expr.mid(pos +1,last-pos -1)),b);
	end_expr.append(QString("(%1)").arg(QString::number(abs(a-b)+1)));
	pos  = ++last;
      }
    }
    end_expr.append(new_expr.mid(last));
  }

  Ami::BinMath op(_input->output(), qPrintable(end_expr));
  
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  source,
						  input_signature,
						  -1,
						  *channels[channel]->filter().filter(),
						  op);
  p += r.size();
  _req.request(r,output);
  _output_signature = r.output();
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
