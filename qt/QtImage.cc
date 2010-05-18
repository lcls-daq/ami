#include "QtImage.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/ImageFrame.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryImage.hh"

#include <QtGui/QImage>

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

  printf("QtImage 0x%x x 0x%x\n",_nx,_ny);

  _qimage = new QImage(_nx, _ny, QImage::Format_Indexed8);
  _qimage->fill(128);

  _xinfo = new AxisBins(d.xlow(),d.xup(),d.nbinsx());
  _yinfo = new AxisBins(d.ylow(),d.yup(),d.nbinsy());
}
  
  
QtImage::QtImage(const QString&   title,
		 const Ami::EntryImage& entry,
		 unsigned x0, unsigned y0,
		 unsigned x1, unsigned y1) :
  QtBase(title,entry)
{
  x0 &= ~3;  // 32-bit aligned
  _x0 = x0; _y0 = y0; _nx = x1-x0+1; _ny = y1-y0+1; 

  _nx = (_nx+3)&~3;

  _qimage = new QImage(_nx, _ny, QImage::Format_Indexed8);
  _qimage->fill(128);

  if (_qimage->isNull())
    printf("image is null\n");
  else
    printf("created with size 0x%x x 0x%x\n",_qimage->size().width(),_qimage->size().height());

  const DescImage& d = entry.desc();
  _xinfo = new AxisBins(d.xlow(),d.xlow() + _nx*d.ppxbin(),_nx);
  _yinfo = new AxisBins(d.ylow(),d.ylow() + _ny*d.ppybin(),_ny);
}
  
  
QtImage::~QtImage()
{
  delete _qimage;
  delete _xinfo;
  delete _yinfo;
}

void           QtImage::dump  (FILE* f) const
{
  const EntryImage& _entry = static_cast<const EntryImage&>(entry());
  fprintf(f,"%d %d\n", _entry.info(EntryImage::Normalization), _entry.info(EntryImage::Pedestal));
  for(unsigned j=_y0; j<_y0+_nx; j++) {
    for(unsigned k=_x0; k<_x0+_nx; k++)
      fprintf(f,"%d ", _entry.content(k,j));
    fprintf(f,"\n");
  }
}

void           QtImage::attach(ImageFrame* p)
{
  p->attach(*this);
}

void           QtImage::update() {}

#define LINTRANS(x) (x > p   ? (x-p)/n : 0)
#define LOGTRANS(x) (x > ppn ? (log(x-p)-logn)*invlogs : 0)

#define COPYIMAGE(type,T,factor) {			      \
    type* dst = (type*)_qimage->bits();			      \
    for(unsigned k=0; k<_ny; k++) {			      \
      for(unsigned j=0; j<_nx; j++) {			      \
        unsigned sh = static_cast<unsigned>(T(*src));	      \
	*dst++ = factor*(sh >= 0xff ? 0xff : sh);	      \
	src++; }					      \
      src += d.nbinsx()-_nx;				      \
    } }

QImage&        QtImage::image(double s, bool linear)
{
  const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
  const Ami::DescImage&  d = _entry.desc();

//   unsigned n = _entry.info(EntryImage::Normalization);
//   n = (n ? n : 1)*d.ppxbin()*d.ppybin();
//   if (s>0)  n<<= s;
//   else      n>>=-s;
  float p = _entry.info(EntryImage::Pedestal);
  float n = entry().desc().isnormalized() ? _entry.info(EntryImage::Normalization) : 1;
  n = (n ? n : 1)*d.ppxbin()*d.ppybin();

  const unsigned* src = _entry.contents() + _y0*d.nbinsx() + _x0;
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

const AxisInfo* QtImage::xinfo() const { return _xinfo; }
const AxisInfo* QtImage::yinfo() const { return _yinfo; }

void QtImage::set_color_table(const QVector<QRgb>& colors) { _qimage->setColorTable(colors); }
