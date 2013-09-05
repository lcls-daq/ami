#include "ZoomPlot.hh"

#include "ami/qt/QtImage.hh"
#include "ami/qt/ImageDisplay.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/ImageGridScale.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/PWidgetManager.hh"

#include "ami/data/Cds.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Zoom.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/BlobFinder.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>

#include "qwt_plot.h"

using namespace Ami::Qt;

#include "ami/qt/Transform.hh"

static Transform* _transform=0;

static Transform& transform() {
  if (!_transform)
    _transform = new Transform(0,"static Transform","y");
  return *_transform;
}

static QColor _color(0,0,0);

//
//  Bug in cross-hair mouse grabbing of zoomed plots.  The grid scale used by
//  the position retrieval function isn't correct.  Don't know how to fix, so
//  disable the mouse grabbing for this case.
//

ZoomPlot::ZoomPlot(QWidget*         parent,
		   const QString&   name,
		   unsigned         input_channel,
		   Ami::AbsOperator* op) :
  QtPWidget(0),
  _name    (name),
  _input   (input_channel),
  _signature(-1),
  _x0     (1),
  _y0     (1),
  _x1     (0),
  _y1     (0),
  _frame  (new ImageDisplay(false)),
  _op     (op)
{
  setWindowTitle(name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();

  PWidgetManager::add(this, _name);
}

ZoomPlot::ZoomPlot(QWidget*         parent,
		   const QString&   name,
		   unsigned         input_channel,
		   unsigned         x0, 
		   unsigned         y0,
		   unsigned         x1,
		   unsigned         y1) :
  QtPWidget(0),
  _name    (name),
  _input   (input_channel),
  _signature(-1),
  _x0     (x0),
  _y0     (y0),
  _x1     (x1),
  _y1     (y1),
  _frame  (new ImageDisplay(false)),
  _op     (0)
{
  setWindowTitle(name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();

  PWidgetManager::add(this, _name);
}

ZoomPlot::ZoomPlot(QWidget*         parent,
		   const QString&   name) :
  QtPWidget(0),
  _name    (name),
  _input   (0),
  _signature(-1),
  _x0     (1),
  _y0     (1),
  _x1     (0),
  _y1     (0),
  _frame  (new ImageDisplay(false)),
  _op     (0)
{
  setWindowTitle(name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();

  PWidgetManager::add(this, _name);
}

ZoomPlot::ZoomPlot(QWidget*         parent,
		   const char*&     p) :
  QtPWidget(0),
  _signature(-1),
  _frame   (new ImageDisplay(false)),
  _op      (0)
{
  load(p);

  setWindowTitle(_name);
  setAttribute(::Qt::WA_DeleteOnClose, true);

  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(_frame);
  setLayout(layout);

  show();

  PWidgetManager::add(this, _name);
}

ZoomPlot::~ZoomPlot()
{
  if (_op) delete _op;
}

void ZoomPlot::save(char*& p) const
{
  char* buff = new char[8*1024];

  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "QString", "_name", QtPersistent::insert(p,_name) );
  XML_insert(p, "unsigned", "_input", QtPersistent::insert(p,_input) );
  XML_insert(p, "unsigned", "_x0", QtPersistent::insert(p,_x0) );
  XML_insert(p, "unsigned", "_y0", QtPersistent::insert(p,_y0) );
  XML_insert(p, "unsigned", "_x1", QtPersistent::insert(p,_x1) );
  XML_insert(p, "unsigned", "_y1", QtPersistent::insert(p,_y1) );
  XML_insert(p, "ImageDisplay", "_frame", _frame->save(p) );
  if (_op) {
    XML_insert( p, "AbsOperator", "_op",
		QtPersistent::insert(p,buff,(char*)_op->serialize(buff)-buff) );
  }
  delete[] buff;
}

void ZoomPlot::load(const char*& p)
{
  if (_op) { 
    delete _op;
    _op = 0;
  }

  XML_iterate_open(p,tag)
    if (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_name")
      _name  = QtPersistent::extract_s(p);
    else if (tag.name == "_input")
      _input = QtPersistent::extract_i(p);
    else if (tag.name == "_x0")
      _x0 = QtPersistent::extract_i(p);
    else if (tag.name == "_y0")
      _y0 = QtPersistent::extract_i(p);
    else if (tag.name == "_x1")
      _x1 = QtPersistent::extract_i(p);
    else if (tag.name == "_y1")
      _y1 = QtPersistent::extract_i(p);
    else if (tag.name == "_frame")
      _frame->load(p);
    else if (tag.name == "_op") {
      const char* v = (const char*)QtPersistent::extract_op(p);
      uint32_t type = (AbsOperator::Type)*reinterpret_cast<const uint32_t*>(v);
      v+=2*sizeof(uint32_t); // type and next
      switch(type) {
      case AbsOperator::PeakFinder       : _op = new PeakFinder       (v); break;
      case AbsOperator::BlobFinder       : _op = new BlobFinder       (v); break;
      default: _op=0; printf("Unable to operator type %d\n",type); break;
      }
    }
  XML_iterate_close(ZoomPlot,tag);
}

void ZoomPlot::setup_payload(Cds& cds)
{
  _frame->reset();
  const Entry* entry = cds.entry(_signature);
  if (entry) {
    if (_x1<_x0) {
      const DescImage& d = static_cast<const DescImage&>(entry->desc());
      unsigned x0 = unsigned(d.xlow());
      unsigned y0 = unsigned(d.ylow());
      unsigned x1 = unsigned(d.xup ());
      unsigned y1 = unsigned(d.yup ());
#if 0
      _frame->add( new QtImage(entry->desc().name(),
                               *static_cast<const EntryImage*>(entry),
                               x0,y0,x1,y1),
                   true);
#else
      _frame->add( new QtImage(entry->desc().name(),
                               *static_cast<const EntryImage*>(entry),
                               transform(), transform(), _color),
                   true);
#endif
    }
    else 
      _frame->add( new QtImage(entry->desc().name(),
                               *static_cast<const EntryImage*>(entry),
                               _x0, _y0, _x1, _y1),
                   true);
    _frame->grid_scale().setup_payload(cds);
  }
}

void ZoomPlot::configure(char*& p, 
                         unsigned input, 
                         unsigned& output,
                         ChannelDefinition* input_channels[],
                         int* input_signatures, 
                         unsigned input_nchannels)
{
  if (_op) {
    ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						    ConfigureRequest::Analysis,
						    input_signatures[_input],
						    -1,
						    *input_channels[_input]->filter().filter(),
						    *_op);
    p += r.size();
    _req.request(r, output);
    _signature = r.output();
  }
  else {
    configure(p, input_signatures[_input], output);
  }
}

void ZoomPlot::configure(char*& p, 
                         unsigned input, 
                         unsigned& output)
{
  _signature = input;
}

void ZoomPlot::update()
{
  _frame  ->update();
}
