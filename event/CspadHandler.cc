#include "CspadHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include "pdsdata/cspad/ElementV1.hh"
#include "pdsdata/cspad/ConfigV1.hh"

#include <string.h>
#include <stdio.h>

//#define ZERO
#define DO_SWAP

typedef Pds::CsPad::ElementV1 CspadElement;

static const double pixel_size = 110e-6;
static const unsigned ppb = 4;
//
//  TwoByTwo origins w.r.t. Quadrant origin
//
static const double _tx0[] = { -44.100e-3, -89.100e-3, -45.100e-3, -44.100e-3 };
static const double _ty0[] = {   0.      ,  44.700e-3,  90.400e-3,  90.400e-3 };
//
//  ASIC origins w.r.t. TwoByTwo origin
//
static const double _ax0[] = { 1.000e-3,  1.000e-3, 22.335e-3, 22.335e-3 };
static const double _ay0[] = { 1.000e-3, 23.488e-3,  1.000e-3, 23.488e-3 };

enum Rotation { D0, D90, D180, D270, NPHI=4 };

static void _transform(double& x,double& y,double dx,double dy,Rotation r)
{
  switch(r) {
  case D0  :    x += dx; y += dy; break;
  case D90 :    x += dy; y -= dx; break;
  case D180:    x -= dx; y -= dy; break;
  case D270:    x -= dy; y += dx; break;
  default:                        break;
  }
}

#ifdef DO_SWAP
#define SWAP_U16(v) ( ((v&0xff00)>>8) | ((v&0x00ff)<<8) )
static inline unsigned sum2(const uint16_t* data)
{
  return ( SWAP_U16(data[0])+SWAP_U16(data[1]) );
}

static unsigned sum4(const uint16_t* data)
{
  return ( SWAP_U16(data[0])+SWAP_U16(data[1])+SWAP_U16(data[2])+SWAP_U16(data[3]) );
}
#else
static inline unsigned sum2(const uint16_t* data)
{
  return data[0]+data[1];
}

static inline unsigned sum4(const uint16_t* data)
{
  return data[0]+data[1]+data[2]+data[3];
}
#endif

namespace CspadGeometry {

  //
  //  When filling the image, compensate data which
  //    only partially fills a pixel (at the edges)
  //
#define FRAME_BOUNDS {							\
    const unsigned ColBins = CsPad::ColumnsPerASIC/ppb;			\
    const unsigned RowBins = CsPad::MaxRowsPerASIC   /ppb;			\
    unsigned x0 = CALC_X(column,0,0);					\
    unsigned x1 = CALC_X(column,ColBins,RowBins);			\
    unsigned y0 = CALC_Y(row,0,0);					\
    unsigned y1 = CALC_Y(row,ColBins,RowBins);				\
    if (x0 > x1) { unsigned t=x0; x0=x1; x1=t; }			\
    if (y0 > y1) { unsigned t=y0; y0=y1; y1=t; }			\
    image.add_frame(x0,y0,x1-x0+1,y1-y0+1);				\
  }

#define BIN_ITER {							\
    const unsigned ColBins = CsPad::ColumnsPerASIC/ppb;			\
    const unsigned RowBins = CsPad::MaxRowsPerASIC   /ppb;		\
    /*  zero the target region  */					\
    for(unsigned i=0; i<=ColBins; i++) {				\
      for(unsigned j=0; j<=RowBins; j++) {				\
	const unsigned x = CALC_X(column,i,j);				\
	const unsigned y = CALC_Y(row   ,i,j);				\
	image.content(0,x,y);						\
      }									\
    }									\
    /*  fill the target region  */					\
    for(unsigned i=0; i<ColBins; i++) {					\
      for(unsigned k=0; k<ppb; k++) {					\
	for(unsigned j=0; j<RowBins; j++) { /* unroll ppb */		\
	  const unsigned x = CALC_X(column,i,j);			\
	  const unsigned y = CALC_Y(row   ,i,j);			\
	  image.addcontent(sum4(data),x,y);				\
	  data += ppb;							\
	}								\
	const unsigned x = CALC_X(column,i,RowBins);			\
	const unsigned y = CALC_Y(row   ,i,RowBins);			\
	image.addcontent(2*sum2(data),x,y);				\
	data += 2;							\
	data += CsPad::MaxRowsPerASIC;					\
      }									\
    }									\
    for(unsigned j=0; j<RowBins; j++) { /* unroll ppb(y) */		\
      const unsigned x = CALC_X(column,ColBins,j);			\
      const unsigned y = CALC_Y(row   ,ColBins,j);			\
      image.addcontent(4*sum4(data),x,y);				\
      data += ppb;							\
    }									\
    const unsigned x = CALC_X(column,ColBins,RowBins);			\
    const unsigned y = CALC_Y(row   ,ColBins,RowBins);			\
    image.addcontent(8*sum2(data),x,y);					\
    data += 2;                     					\
    data += CsPad::MaxRowsPerASIC;    					\
}

  //
  //  This class locates the ASIC data to the binned image grid
  //
  class Asic {
  public:
    Asic() {}
    Asic(double x, double y, Rotation r) :
      column(unsigned( x/pixel_size)/ppb),
      row   (unsigned(-y/pixel_size)/ppb),
      orientation(r) {}
    virtual ~Asic() {}
  public:
    void fill(Ami::DescImage& image) const
    {
      switch(orientation) {
      case D0:
#define CALC_X(a,b,c) (a+b)
#define CALC_Y(a,b,c) (a-c)
      FRAME_BOUNDS
#undef CALC_X
#undef CALC_Y
	break;
      case D90:
#define CALC_X(a,b,c) (a+c)
#define CALC_Y(a,b,c) (a+b)
      FRAME_BOUNDS
#undef CALC_X
#undef CALC_Y
	break;
      case D180:
#define CALC_X(a,b,c) (a-b)
#define CALC_Y(a,b,c) (a+c)
      FRAME_BOUNDS
#undef CALC_X
#undef CALC_Y
	break;
      case D270:
#define CALC_X(a,b,c) (a-c)
#define CALC_Y(a,b,c) (a-b)
      FRAME_BOUNDS
#undef CALC_X
#undef CALC_Y
	break;
      default:
	break;
      }
    }
    void fill(Ami::EntryImage& image,
	      const uint16_t*  data) const
    {
      switch(orientation) {
      case D0:
#define CALC_X(a,b,c) (a+b)
#define CALC_Y(a,b,c) (a-c)
      BIN_ITER
#undef CALC_X
#undef CALC_Y
	break;
      case D90:
#define CALC_X(a,b,c) (a+c)
#define CALC_Y(a,b,c) (a+b)
      BIN_ITER
#undef CALC_X
#undef CALC_Y
	break;
      case D180:
#define CALC_X(a,b,c) (a-b)
#define CALC_Y(a,b,c) (a+c)
      BIN_ITER
#undef CALC_X
#undef CALC_Y
	break;
      case D270:
#define CALC_X(a,b,c) (a-c)
#define CALC_Y(a,b,c) (a-b)
      BIN_ITER
#undef CALC_X
#undef CALC_Y
	break;
      default:
	break;
      }
    }
  private:
    unsigned column, row;
    Rotation orientation;
  };

  class TwoByTwo {
  public:
    TwoByTwo() {}
    TwoByTwo(double x, double y, Rotation r) 
    {
      for(unsigned i=0; i<4; i++) {
	double tx(x), ty(y);
	_transform(tx,ty,_ax0[i],_ay0[i],r);
	asic[i] = Asic(tx,ty,r);
      }
    }
    void fill(Ami::DescImage& image) const
    {
      asic[0].fill(image);
      asic[1].fill(image);
      asic[2].fill(image);
      asic[3].fill(image);
    }
    void fill(Ami::EntryImage& image,
	      const uint16_t*  data) const
    {
      asic[0].fill(image,data);
      asic[1].fill(image,data+194);
      asic[2].fill(image,data+194*370);
      asic[3].fill(image,data+194*371);
    }
  public:
    Asic asic[4];
  };

  class Quad {
  public:
    Quad() {}
    Quad(double x, double y, Rotation r)
    {
      static Rotation _tr[] = {  D0  , D90 , D180, D90 ,
				 D90 , D180, D270, D180,
				 D180, D270, D0  , D270,
				 D270, D0  , D90 , D0 };
      for(unsigned i=0; i<4; i++) {
	double tx(x), ty(y);
	_transform(tx,ty,_tx0[i],_ty0[i],r);
	element[i] = TwoByTwo( tx, ty, _tr[r*NPHI+i] );
      }
    }
  public:
    void fill(Ami::DescImage&    image) const
    {
      element[0].fill(image);
      element[1].fill(image);
      element[2].fill(image);
      element[3].fill(image);
    }
    void fill(Ami::EntryImage&    image,
	      const CspadElement* data,
	      unsigned            mask) const
    {
      const uint16_t* d = data->data();
      for(unsigned i=0; i<4; i++, 
	    d+=4*Pds::CsPad::ColumnsPerASIC*Pds::CsPad::MaxRowsPerASIC)
	if (mask&(1<<i)) element[i].fill(image,d);
    }
  public:
    TwoByTwo element[4];
  };

  class Detector {
  public:
    Detector()
    {
      double x =  0.5*double(xpixels())*pixel_size;
      double y = -0.5*double(ypixels())*pixel_size;
      //
      //  Stack the quads against each other
      //
      quad[0] = Quad(x,y,D0  );
      quad[1] = Quad(x,y,D90 );
      quad[2] = Quad(x,y,D180);
      quad[3] = Quad(x,y,D270);
    }
  public:
    void set_configuration(const Pds::CsPad::ConfigV1& c)
    {
      _config = c;
      printf("CspadHandler found configuration with quad mask %x  asic mask %x\n",
	     c.quadMask(),c.asicMask());
    }
    void fill(Ami::DescImage&    image) const
    {
      //
      //  The configuration should tell us how many elements to view
      //
      for(unsigned i=0; i<4; i++)
	if (_config.quadMask() & (1<<i))
	  quad[i].fill(image);
    }
    void fill(Ami::EntryImage&    image,
	      const CspadElement* data) const
    {
      //
      //  The configuration should tell us how many elements to view
      //
      unsigned qmask = _config.quadMask();
      while(qmask) {
	quad[data->quad()].fill(image,data,_config.asicMask());
	qmask &= ~(1<<data->quad());
	data = data->next(_config);
      }
    }
    static unsigned xpixels() { return 2048-256; }
    static unsigned ypixels() { return 2048-256; }
  private:
    Quad quad[4];
    Pds::CsPad::ConfigV1 _config;
  };
};

using namespace Ami;

CspadHandler::CspadHandler(const Pds::DetInfo& info) :
  EventHandler(info, Pds::TypeId::Id_CspadElement, Pds::TypeId::Id_CspadConfig),
  _entry(0),
  _detector(new CspadGeometry::Detector)
{
}

CspadHandler::CspadHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
  EventHandler(info, Pds::TypeId::Id_CspadElement, Pds::TypeId::Id_CspadConfig),
  _entry(entry ? new EntryImage(entry->desc()) : 0),
  _detector(new CspadGeometry::Detector)
{
}

CspadHandler::~CspadHandler()
{
  delete _detector;
  if (_entry)
    delete _entry;
}

unsigned CspadHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* CspadHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void CspadHandler::reset() { _entry = 0; }

void CspadHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  _detector->set_configuration(*reinterpret_cast<const Pds::CsPad::ConfigV1*>(payload));

  if (_entry) 
    delete _entry;

  const DetInfo& det = static_cast<const DetInfo&>(info());
  DescImage desc(det, 0, ChannelID::name(det,0),
		 _detector->xpixels()/ppb, _detector->ypixels()/ppb, ppb, ppb);
  _detector->fill(desc);

  _entry = new EntryImage(desc);
  memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
  _entry->info(0,EntryImage::Pedestal);
  _entry->info(0,EntryImage::Normalization);
  _entry->invalid();
}

void CspadHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}

void CspadHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  const CspadElement* e = reinterpret_cast<const CspadElement*>(payload);
  _detector->fill(*_entry,e);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
}

void CspadHandler::_damaged() { _entry->invalid(); }
