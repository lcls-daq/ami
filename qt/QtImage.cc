#include "QtImage.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/ImageGrid.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryImage.hh"

#include <QtGui/QImage>
#include <QtGui/QGridLayout>

#include <stdio.h>

using namespace Ami::Qt;

QtImage::QtImage(const QString&   title,
		 const Ami::EntryImage& entry,
		 const AbsTransform& x,
		 const AbsTransform& y,
		 const QColor& c) :
  QtBase(title,entry)
{
  const Ami::DescImage& d = entry.desc();
  _x0 = 0; _y0 = 0; _nx = d.nbinsx(); _ny = d.nbinsy();

  _qimage = new QImage(_nx, _ny, QImage::Format_Indexed8);
  _qimage->fill(128);

  _xinfo = new AxisBins(d.xlow(),d.xup(),d.nbinsx());
  _yinfo = new AxisBins(d.ylow(),d.yup(),d.nbinsy());

  _scalexy = false;
  _xgrid = new ImageGrid(ImageGrid::X, ImageGrid::TopLeft, 
                         d.binx(_x0), double(d.ppxbin()*_nx), _nx);
  _ygrid = new ImageGrid(ImageGrid::Y, ImageGrid::TopLeft, 
                         d.biny(_y0), double(d.ppybin()*_ny), _ny);
}
  
QtImage::~QtImage()
{
  delete _qimage;
  delete _xinfo;
  delete _yinfo;
  delete _xgrid;
  delete _ygrid;
}

void           QtImage::dump  (FILE* f) const
{
  const EntryImage& _entry = static_cast<const EntryImage&>(entry());
  fprintf(f,"%f %f\n", _entry.info(EntryImage::Normalization), _entry.info(EntryImage::Pedestal));
  ndarray<const uint32_t,2> data(_entry.content());
  for(unsigned j=_y0; j<_y0+_ny; j++) {
    for(unsigned k=_x0; k<_x0+_nx; k++)
      fprintf(f,"%d ", data[j][k]);
    fprintf(f,"\n");
  }
}

void           QtImage::attach(ImageFrame* p)
{
  p->attach(this);
}

void           QtImage::update() {}

void           QtImage::canvas_resize(const QSize& sz)
{
  _xgrid->resize(sz.width ());
  _ygrid->resize(sz.height());
}

void           QtImage::canvas_size(const QSize& sz,
                                    QGridLayout& layout)
{
  if (_scalexy) {
    _xgrid->resize(sz.width ());
    _ygrid->resize(sz.height());
  }
  layout.addWidget(_xgrid,1,0);
  layout.addWidget(_ygrid,0,1);
}


#define LINTRANS(x) (x > p   ? (x-p)/n : 0)
#define LOGTRANS(x) (x > ppn ? (log(x-p)-logn)*invlogs : 0)

#define COPYIMAGE(type,T,factor) {			      \
    ndarray<const uint32_t,2> src(_entry.content());          \
    for(unsigned k=0; k<_ny; k++) {			      \
      type* dst = (type*)_qimage->scanLine(k);		      \
      for(unsigned j=_x0; j<_x0+_nx; j++) {                   \
        unsigned sh = unsigned(T(src[_y0+k][j]));             \
	*dst++ = factor*(sh >= 0xff ? 0xff : sh);	      \
      }                                                       \
    } }

QImage&        QtImage::image(float p0, float s, bool linear)
{
  const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
  const Ami::DescImage&  d = _entry.desc();

//   unsigned n = _entry.info(EntryImage::Normalization);
//   n = (n ? n : 1)*d.ppxbin()*d.ppybin();
//   if (s>0)  n<<= s;
//   else      n>>=-s;
  float p = _entry.info(EntryImage::Pedestal);
  float n = entry().desc().isnormalized() ? _entry.info(EntryImage::Normalization) : 1;
  if (!d.countmode())
    n = (n ? n : 1)*d.ppxbin()*d.ppybin();
  p += float(n)*p0;

  if (linear) {
    n *= double(s);
    COPYIMAGE(uint8_t ,LINTRANS,0x01);
  }
  else {
    const float  ppn  = p+n;
    const double logn = log(n);
    const double invlogs = 256./(log(s)+log(256));
    COPYIMAGE(uint8_t ,LOGTRANS,0x01);
  }

  return *_qimage;
}

void QtImage::xscale_update() {}
void QtImage::yscale_update() {}

float QtImage::value(unsigned x,unsigned y) const // units are bins
{
  const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
  const Ami::DescImage&  d = _entry.desc();
  float p = _entry.info(EntryImage::Pedestal);
  float n = entry().desc().isnormalized() ? _entry.info(EntryImage::Normalization) : 1;
  if (!d.countmode())
    n = (n ? n : 1)*d.ppxbin()*d.ppybin();
  return (float(_entry.content(_x0+x,_y0+y))-p)/n;
 }

const AxisInfo* QtImage::xinfo() const { return _xinfo; }
const AxisInfo* QtImage::yinfo() const { return _yinfo; }

bool QtImage::scalexy() const { return _scalexy; }
void QtImage::scalexy(bool v) { _scalexy = v; }

void QtImage::set_color_table(const QVector<QRgb>& colors) { _qimage->setColorTable(colors); }

void QtImage::set_grid_scale(double scalex, double scaley)
{
  const DescImage& d = static_cast<const EntryImage&>(entry()).desc();
  _xgrid->set_scale(scalex*d.binx(_x0),
                    scalex*double(d.ppxbin()*_nx));
  _ygrid->set_scale(scaley*d.biny(_y0),
                    scaley*double(d.ppybin()*_ny));
}
