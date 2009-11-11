#include "QtImage.hh"
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
  _x0 = 0; _y0 = 0; _nx = d.nbinsx(); _ny = d.nbinsy(); _scale = 1;

  printf("QtImage 0x%x x 0x%x\n",_nx*_scale,_ny*_scale);

  _qimage = new QImage(_nx*_scale, _ny*_scale, QImage::Format_Indexed8);
  _qimage->fill(128);
}
  
  
QtImage::QtImage(const QString&   title,
		 const Ami::EntryImage& entry,
		 unsigned x0, unsigned y0,
		 unsigned x1, unsigned y1) :
  QtBase(title,entry)
{
  x0 &= ~3;  // 32-bit aligned
  _x0 = x0; _y0 = y0; _nx = x1-x0+1; _ny = y1-y0+1; 

  _scale = 1;
#ifdef FAST_IMPLEMENTATION
  unsigned sx = entry.desc().nbinsx() / _nx;
  unsigned sy = entry.desc().nbinsy() / _ny;
  unsigned s = (sx < sy ) ? sx : sy;
  while( s>>=1 )
    _scale<<=1;
  if (_scale>8) _scale=8;

  if (_scale==1) _nx = (_nx+3)&~3;
  if (_scale==2) _nx = (_nx+1)&~1;
#else
  _nx = (_nx+3)&~3;
#endif

  printf("QtImage 0x%x x 0x%x\n",_nx*_scale,_ny*_scale);

  _qimage = new QImage(_nx*_scale, _ny*_scale, QImage::Format_Indexed8);
  _qimage->fill(128);

  if (_qimage->isNull())
    printf("image is null\n");
  else
    printf("created with size 0x%x x 0x%x\n",_qimage->size().width(),_qimage->size().height());
}
  
  
QtImage::~QtImage()
{
  delete _qimage;
}

void           QtImage::dump  (FILE* f) const
{
  const EntryImage& _entry = static_cast<const EntryImage&>(entry());
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

#ifdef FAST_IMPLEMENTATION

#define NO_SHIFT (*src++)
#define LEFT_SHIFT ((*src++)<<shift)
#define RIGHT_SHIFT ((*src++)>>shift)

#define COPYIMAGE(type,shift_op,factor) {		\
    type* dst = (type*)_qimage->bits();			\
    for(unsigned k=0; k<_ny; k++) {			\
      type* r = dst;					\
      for(unsigned j=0; j<_nx; j++) {			\
	unsigned sh = shift_op;				\
	*dst++ = factor*(sh >= 0xff ? 0xff : sh); }	\
      for(unsigned j=1; j<_scale; j++) {		\
	memcpy(dst, r, _nx*sizeof(type));		\
	dst += _nx; }					\
      src += d.nbinsx()-_nx;				\
    } }

QImage&        QtImage::image(int s)
{
  const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
  const Ami::DescImage&  d = _entry.desc();

  //  int shift(s);
  //  unsigned n = _entry.info(EntryImage::Normalization);
  //  n = (n ? n : 1)*d.ppxbin()*d.ppybin();
  //  while( (n>>=1) )
  //    shift++;

  const unsigned* src = _entry.contents() + _y0*d.nbinsx() + _x0;
  if (shift>0) {
    switch(_scale) {
    case 1: { COPYIMAGE(uint8_t ,RIGHT_SHIFT,0x01); break; }
    case 2: { COPYIMAGE(uint16_t,RIGHT_SHIFT,0x0101); break; }
    case 4: { COPYIMAGE(uint32_t,RIGHT_SHIFT,0x01010101); break; }
    case 8: { COPYIMAGE(uint64_t,RIGHT_SHIFT,0x0101010101010101ULL); break; }
    default: break;
    }
  }
  else if (shift<0) {
    shift = -shift;
    switch(_scale) {
    case 1: { COPYIMAGE(uint8_t ,LEFT_SHIFT,0x01); break; }
    case 2: { COPYIMAGE(uint16_t,LEFT_SHIFT,0x0101); break; }
    case 4: { COPYIMAGE(uint32_t,LEFT_SHIFT,0x01010101); break; }
    case 8: { COPYIMAGE(uint64_t,LEFT_SHIFT,0x0101010101010101ULL); break; }
    default: break;
    }
  }
  else {
    switch(_scale) {
    case 1: { COPYIMAGE(uint8_t ,NO_SHIFT,0x01); break; }
    case 2: { COPYIMAGE(uint16_t,NO_SHIFT,0x0101); break; }
    case 4: { COPYIMAGE(uint32_t,NO_SHIFT,0x01010101); break; }
    case 8: { COPYIMAGE(uint64_t,NO_SHIFT,0x0101010101010101ULL); break; }
    default: break;
    }
  }

  return *_qimage;
}
#else

#define COPYIMAGE(type,factor) {			\
    type* dst = (type*)_qimage->bits();			\
    for(unsigned k=0; k<_ny; k++) {			\
      for(unsigned j=0; j<_nx; j++) {			\
        unsigned sh = static_cast<unsigned>((*src++)/n);       \
	*dst++ = factor*(sh >= 0xff ? 0xff : sh); }	\
      src += d.nbinsx()-_nx;				\
    } }

QImage&        QtImage::image(double s)
{
  const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
  const Ami::DescImage&  d = _entry.desc();

//   unsigned n = _entry.info(EntryImage::Normalization);
//   n = (n ? n : 1)*d.ppxbin()*d.ppybin();
//   if (s>0)  n<<= s;
//   else      n>>=-s;
  float n = entry().desc().isnormalized() ? _entry.info(EntryImage::Normalization) : 1;
  n = (n ? n : 1)*d.ppxbin()*d.ppybin();
  n *= double(s);

  const unsigned* src = _entry.contents() + _y0*d.nbinsx() + _x0;
  switch(_scale) {
    case 1: { COPYIMAGE(uint8_t ,0x01); break; }
    case 2: { COPYIMAGE(uint16_t,0x0101); break; }
    case 4: { COPYIMAGE(uint32_t,0x01010101); break; }
    case 8: { COPYIMAGE(uint64_t,0x0101010101010101ULL); break; }
    default: break;
    }
  return *_qimage;
}
#endif

void QtImage::xscale_update() {}
void QtImage::yscale_update() {}

const AxisInfo* QtImage::xinfo() const { return 0; }
const AxisInfo* QtImage::yinfo() const { return 0; }

void QtImage::set_color_table(const QVector<QRgb>& colors) { _qimage->setColorTable(colors); }
