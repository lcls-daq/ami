#include "QtImage.hh"
#include "ami/qt/AxisPixels.hh"
#include "ami/qt/AxisPixelsR.hh"
#include "ami/qt/ImageFrame.hh"
#include "ami/qt/ImageGrid.hh"
#include "ami/qt/ImageColorControl.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ImageMask.hh"
#include "ami/service/DataLock.hh"
#include "ami/service/Semaphore.hh"

#include "psalg/psalg.h"

#include <QtGui/QImage>
#include <QtGui/QGridLayout>

#include <stdio.h>
#include <climits>

using namespace Ami::Qt;

QtImage::QtImage(const QString&   title,
		 const Ami::EntryImage& entry,
		 const AbsTransform& x,
		 const AbsTransform& y,
		 const QColor& c,
                 DataLock* lock) :
  QtBase(title,entry),
  _lock (lock),
  _sem  (lock ? lock->read_register():0)
{
  const Ami::DescImage& d = entry.desc();

  for(unsigned i=0; i<NBUFFERS; i++) {
    _qimage[i] = new QImage(d.ndispx(), d.ndispy(), QImage::Format_Indexed8);
    _qimage[i]->fill(128);
  }
  _mimage = (1<<NBUFFERS)-1;

  if (d.xup()-d.xlow() >= float(d.ndispx())) {
    _xinfo = new AxisPixels(d.xlow(),d.xup(),d.ndispx());
    _yinfo = new AxisPixels(d.ylow(),d.yup(),d.ndispy());
  }
  else {
    _xinfo = new AxisPixelsR(d.xlow(),d.xup(),d.ndispx());
    _yinfo = new AxisPixelsR(d.ylow(),d.yup(),d.ndispy());
  }

  _scalexy = false;
  _xgrid = new ImageGrid(ImageGrid::X, ImageGrid::TopLeft, 
                         d.binx(0), double(d.ppxbin()*d.nbinsx()), d.ndispx());
  _ygrid = new ImageGrid(ImageGrid::Y, ImageGrid::TopLeft, 
                         d.biny(0), double(d.ppybin()*d.nbinsy()), d.ndispy());
}
  
QtImage::~QtImage()
{
  if (_lock) _lock->read_resign(_sem);

  for(unsigned i=0; i<NBUFFERS; i++)
    delete _qimage[i];
  delete _xinfo;
  delete _yinfo;
  delete _xgrid;
  delete _ygrid;
}

void           QtImage::dump  (FILE* f) const
{
  const EntryImage& _entry = static_cast<const EntryImage&>(entry());
  const DescImage& d = _entry.desc();
  fprintf(f,"%f %f\n", _entry.info(EntryImage::Normalization), _entry.info(EntryImage::Pedestal));
  ndarray<const uint32_t,2> data(_entry.content());
  for(unsigned j=0; j<d.nbinsy(); j++) {
    for(unsigned k=0; k<d.nbinsx(); k++)
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
    _xgrid->resize(sz.width ()-1);  // Need to subtract 1 to get proper size (don't know why)
    _ygrid->resize(sz.height());
  }
  layout.addWidget(_xgrid,1,0);
  layout.addWidget(_ygrid,0,1);
}


#define LINTRANS(x) (x > p   ? (x-p)/n : 0)
#define LOGTRANS(x) (x > ppn ? (log(x-p)-logn)*invlogs : 0)

QImage*  QtImage::image(ImageColorControl& control)
{
  if (_sem) _sem->take();

  if (!entry().valid()) {
#ifdef DBUG
    printf("QtImage::image invalid\n");
#endif
    //    return 0;
    //  If I return here, then images that don't contribute to every event
    //  may not display.
  }

  QImage* qimage=0;
  for(unsigned i=0; i<NBUFFERS; i++)
    if (_mimage & (1<<i)) {
      qimage = _qimage[i];
      _mimage &= ~(1<<i);
      break;
    }

  if (qimage) {
    const Ami::EntryImage& _entry = static_cast<const Ami::EntryImage&>(entry());
    const Ami::DescImage&  d = _entry.desc();
    const ImageMask* mask = d.mask();

    //   unsigned n = _entry.info(EntryImage::Normalization);
    //   n = (n ? n : 1)*d.ppxbin()*d.ppybin();
    //   if (s>0)  n<<= s;
    //   else      n>>=-s;
    float p = _entry.info(EntryImage::Pedestal);
    float n = entry().desc().isnormalized() ? _entry.info(EntryImage::Normalization) : 1;
    if (!d.countmode())
      n = (n ? n : 1)*d.ppxbin()*d.ppybin();

    switch(control.range()) {
    case ImageColorControl::Fixed: break;
    case ImageColorControl::Full:
      { unsigned vshape[] = {2};
        ndarray<unsigned,1> v(vshape);
        if (mask)
          v = psalg::extremes(_entry.content(), mask->row_mask(), mask->all_mask());
        else if (d.nframes()) {
          v[0] = UINT_MAX;
          v[1] = 0;
          for(unsigned fn=0; fn<d.nframes(); fn++) {
            ndarray<unsigned,1> fv = psalg::extremes(_entry.contents(fn));
            if (fv[0]<v[0]) v[0]=fv[0];
            if (fv[1]>v[1]) v[1]=fv[1];
          }
        }
        else
          v = psalg::extremes(_entry.content());
        control.full_range_setup((double(v[0])-p)/n,
                                 (double(v[1])-p)/n);
      } break;
    case ImageColorControl::Dynamic:
      { unsigned vshape[] = {5};
        ndarray<double,1> v(vshape);
        if (mask)
          v = psalg::moments(_entry.content(), mask->row_mask(), mask->all_mask(),p);
        else if (d.nframes()) {
          memset(v.data(),0,v.size()*sizeof(unsigned));
          for(unsigned fn=0; fn<d.nframes(); fn++) {
            ndarray<double,1> fv = psalg::moments(_entry.contents(fn),p);
            for(unsigned j=0; j<v.size(); j++)
              v[j] += fv[j];
          }
        }
        else
          v = psalg::moments(_entry.content(),p);
        control.dynamic_range_setup(v[0],v[1]/n,v[2]/(n*n));
      } break;
    default: break;
    }

    float p0 = control.pedestal();
    float s  = control.scale();
    bool linear = control.linear();
    
    p += float(n)*p0;
    
    if (linear) {
      n *= double(s);

      ndarray<const uint32_t,2> src(_entry.content());	      
      if (d.disppbx()==1 && d.disppby()==1) {
	for(unsigned k=0; k<d.nbinsy(); k++) {			      
	  uint8_t* dst = (uint8_t*)qimage->scanLine(k);		      
	  for(unsigned j=0; j<d.nbinsx(); j++) {		      
	    unsigned sh = unsigned(LINTRANS(src[k][j]));	      
	    *dst++ = 0x01*(sh >= 0xff ? 0xff : sh);	      
	  }						      
	}
      }
      else {
	for(unsigned k=0; k<d.ndispy(); k++) {			      
	  uint8_t* dst = (uint8_t*)qimage->scanLine(k);		      
	  unsigned ks = k/d.disppby();
	  for(unsigned j=0; j<d.nbinsx(); j++) {		      
	    unsigned sh = unsigned(LINTRANS(src[ks][j]));	      
	    const uint8_t v = 0x01*(sh >= 0xff ? 0xff : sh);	      
	    for(unsigned m=0; m<d.disppbx(); m++)
	      *dst++ = v;
	  }						      
	}
      }
    }
    else {
      const float  ppn  = p+n;
      const double logn = log(n);
      const double invlogs = 256./(log(s)+log(256));

      ndarray<const uint32_t,2> src(_entry.content());	      
      if (d.disppbx()==1 && d.disppby()==1) {
	for(unsigned k=0; k<d.nbinsy(); k++) {			      
	  uint8_t* dst = (uint8_t*)qimage->scanLine(k);		      
	  for(unsigned j=0; j<d.nbinsx(); j++) {		      
	    unsigned sh = unsigned(LOGTRANS(src[k][j]));	      
	    *dst++ = 0x01*(sh >= 0xff ? 0xff : sh);	      
	  }		
	}				      
      }
      else {
	for(unsigned k=0; k<d.ndispy(); k++) {			      
	  uint8_t* dst = (uint8_t*)qimage->scanLine(k);		      
	  unsigned ks=k/d.disppby();
	  for(unsigned j=0; j<d.nbinsx(); j++) {		      
	    unsigned sh = unsigned(LOGTRANS(src[ks][j]));	      
	    const uint8_t v = 0x01*(sh >= 0xff ? 0xff : sh);	      
	    for(unsigned m=0; m<d.disppbx(); m++)
	      *dst++ = v;
	  }		
	}				      
      }
    }
  }

  if (_sem) _sem->give();

  return qimage;
}

bool QtImage::owns   (QImage* q) const
{
  for(unsigned i=0; i<NBUFFERS; i++)
    if (_qimage[i]==q) return true;
  return false;
}

void QtImage::release(QImage* q) 
{
  for(unsigned i=0; i<NBUFFERS; i++)
    if (_qimage[i]==q)
      _mimage |= (1<<i);
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
  return (float(_entry.content(x,y))-p)/n;
 }

const AxisInfo* QtImage::xinfo() const { return _xinfo; }
const AxisInfo* QtImage::yinfo() const { return _yinfo; }

bool QtImage::scalexy() const { return _scalexy; }
void QtImage::scalexy(bool v) { _scalexy = v; }

void QtImage::set_color_table(const QVector<QRgb>& colors) 
{
  for(unsigned i=0; i<NBUFFERS; i++)
    _qimage[i]->setColorTable(colors);
}

void QtImage::set_grid_scale(double scalex, double scaley)
{
  const DescImage& d = static_cast<const EntryImage&>(entry()).desc();
  _xgrid->set_scale(scalex*d.binx(0),
                    scalex*double(d.ppxbin()*d.nbinsx()));
  _ygrid->set_scale(scaley*d.biny(0),
                    scaley*double(d.ppybin()*d.nbinsy()));
}
