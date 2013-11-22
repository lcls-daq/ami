#include "MaskFrame.hh"

#include "ami/qt/AxisInfo.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/Cursors.hh"
#include "ami/qt/ImageXYControl.hh"
#include "ami/qt/ImageColorControl.hh"
#include "ami/qt/ImageMarker.hh"
#include "ami/qt/QtImage.hh"
#include "ami/qt/MaskDisplay.hh"
#include "ami/data/ImageMask.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QMouseEvent>

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QPaintEngine>
#include <QtGui/QScrollArea>

#define USE_SCROLL

using namespace Ami::Qt;

static const int CanvasSizeDefault  = 512;
static const int CanvasSizeIncrease = 4;

MaskFrame::MaskFrame(MaskDisplay* parent,
                     const ImageXYControl& xycontrol,
                     const ImageColorControl& control) : 
  QWidget(parent), 
  _parent   (parent),
  _engine   (*this,control),
  _xycontrol(xycontrol),
  _scroll_area(new QScrollArea),
  _canvas(new QLabel),
  _mask  (0),
  _c(0)
{
  //  const unsigned sz = CanvasSizeDefault + CanvasSizeIncrease;
  //  _canvas->setMinimumSize(sz,sz);
  _canvas->setAlignment(::Qt::AlignLeft | ::Qt::AlignTop);
#if 0
  const unsigned sz = CanvasSizeDefault + CanvasSizeIncrease;
  _canvas->setMinimumSize(sz,sz);
#endif

#ifdef USE_SCROLL
  { QWidget* w = new QWidget(0);
    QGridLayout* layout = new QGridLayout;
    layout->addWidget(_canvas,0,0);
    w->setLayout(layout);
    _scroll_area->setWidget(w);

    _canvas->setMouseTracking(true);
    w->setMouseTracking(true);
    _scroll_area->setMouseTracking(true);
    setMouseTracking(true); }

  _scroll_area->setWidgetResizable(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(_scroll_area);
  setLayout(layout);
#else
  QGridLayout* layout = new QGridLayout;
  layout->addWidget(_canvas,0,0);
  setLayout(layout);
#endif

  connect(&_xycontrol , SIGNAL(windowChanged()), this , SLOT(replot()));
  connect(&control    , SIGNAL(windowChanged()), this , SLOT(scale_changed()));
  connect(this        , SIGNAL(changed()), this, SLOT(replot()));
}

MaskFrame::~MaskFrame() {}

void MaskFrame::attach_mask(ImageMask& mask)
{
  _mask = &mask;
}

void MaskFrame::attach_bkg (QtImage& image) 
{
  _engine.qimage(&image);

  image.set_color_table(_engine.control().color_table());

  QSize sz(_canvas->size());
  sz.rwidth()  += CanvasSizeIncrease;
  sz.rheight() += CanvasSizeIncrease;
#ifdef USE_SCROLL
  QGridLayout* l = static_cast<QGridLayout*>(_scroll_area->widget()->layout()); 
  image.canvas_size(sz,*l);
  _scroll_area->updateGeometry();
#else
  QGridLayout* l = static_cast<QGridLayout*>(layout());
  image.canvas_size(sz,*l);
#endif
}

void MaskFrame::attach_marker(ImageMarker& marker)
{
  _markers.clear();
  _markers.push_back(&marker);
}

void MaskFrame::scale_changed()
{
  if (_engine.qimage()) _engine.qimage()->set_color_table(_engine.control().color_table());
  replot();
}

static int mask_color(int c)
{
  return (c+256)/2;
}

void MaskFrame::replot()
{
  if (!_canvas->isVisible())
    return;

  _engine.render();
}

void MaskFrame::render_image(QImage& output)
{
  //  Draw the markers
  for(std::list<ImageMarker*>::iterator it=_markers.begin(); it!=_markers.end(); it++)
    (*it)->draw(output);
    
    //  Apply mask to the image
  for(int i=0; i<output.height(); i++) {
    uchar* p = output.scanLine(i);
    for(int j=0; j<output.bytesPerLine(); j++,p++) {
      *p &= 0xfe; 
      if (_mask->rowcol(i,j))
        *p |= 1;
    }
  }

  //  Apply the masking color table
  {
    QVector<QRgb> colors(_engine.control().color_table());
    for(int i=1; i<colors.size(); i+=2)
      colors[i] = qRgb( mask_color(qRed(colors[i])),
                        mask_color(qGreen(colors[i])), 
                        mask_color(qBlue(colors[i])) );
    output.setColorTable(colors);
  }
}

void MaskFrame::render_pixmap(QImage& output)
{
  _canvas->setPixmap(QPixmap::fromImage(output).scaled(output.size()*_xycontrol.scale(),
                                                       ::Qt::KeepAspectRatio));

  QSize sz(output.size()*_xycontrol.scale());
  sz.rwidth()  += CanvasSizeIncrease;
  sz.rheight() += CanvasSizeIncrease;
  _engine.qimage()->canvas_resize(sz);
}

void MaskFrame::mousePressEvent(QMouseEvent* e)
{
  QPoint p3 = _canvas->mapFromGlobal( mapToGlobal(e->pos()) );
  double scale = _xycontrol.scale();
  if (_c && _engine.qimage()) {
    if (e->button()==::Qt::RightButton)
      _c->mousePressRight(double(p3.x())/scale,
                          double(p3.y())/scale);
    else
      _c->mousePressEvent(double(p3.x())/scale,
                          double(p3.y())/scale);
    QWidget::mousePressEvent(e);
    emit changed();
  }
  else
    QWidget::mousePressEvent(e);
}

void MaskFrame::mouseMoveEvent(QMouseEvent* e)
{
  QPoint p3 = _canvas->mapFromGlobal( mapToGlobal(e->pos()) );
  double scale = _xycontrol.scale();
  int ix=int(double(p3.x())/scale);
  int iy=int(double(p3.y())/scale);
  _parent->mouse_pos(ix,iy);
  if (_c) {
    _c->mouseMoveEvent(ix,iy);
    QWidget::mousePressEvent(e);
    emit changed();
  }
  else
    QWidget::mouseMoveEvent(e);
}

void MaskFrame::mouseReleaseEvent(QMouseEvent* e)
{
  QPoint p3 = _canvas->mapFromGlobal( mapToGlobal(e->pos()) );
  double scale = _xycontrol.scale();
  if (_c) {
    _c->mouseReleaseEvent(double(p3.x())/scale,
			  double(p3.y())/scale);
    QWidget::mouseReleaseEvent(e);
    emit changed();
  }
  else
    QWidget::mouseReleaseEvent(e);

}

void MaskFrame::set_cursor_input(Cursors* c) 
{
  _c=c; 
}

static AxisBins _defaultInfo(2,0,1);

const AxisInfo* MaskFrame::xinfo() const { 
  return _engine.qimage() ? _engine.qimage()->xinfo() : &_defaultInfo; 
}

const AxisInfo* MaskFrame::yinfo() const { 
  return _engine.qimage() ? _engine.qimage()->yinfo() : &_defaultInfo;
}


