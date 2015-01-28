#include "ami/data/LeastSquares.hh"

#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"

#include <float.h>

namespace Ami {
  class LeastSquaresStats {
  public:
    LeastSquaresStats(double& n,
		      double& xp,
		      double& xxp,
		      double& yp,
		      double& xyp) :
      _n(n), _x(xp), _xx(xxp), _y(yp), _xy(xyp) {}
  public:
    void init (double x, double y, double* a) {
      _n  =1;
      _x  =x;
      _xx =x*x;
      _y  =y;
      _xy =x*y;
      
      a[0] =DBL_MIN;
      a[1] =0;
      a[2] =0;
    }
    void accum(double x, double y, double* a) {
      _n++;
      _x  += x;
      _xx += x*x;
      _y  += y;
      _xy += x*y;

      double disc = (_n*_xx - _x*_x);
      if (disc > 0) {
	double beta = (_n*_xy - _x*_y) / disc; 
	a[0] = _n;
	a[1] = _n * beta;
	a[2] = _y - beta*_x;
      }
      else {
	a[0] = DBL_MIN;
	a[1] = 0;
	a[2] = 0;
      }
    }
  private:
    double &_n, &_x, &_xx, &_y, &_xy;
  };
};

using namespace Ami;

LeastSquares::LeastSquares(const DescEntry& o)
{
#define CASE(t,nx,ny)						 \
  case DescEntry::t:						 \
    { const Desc##t& d = static_cast<const Desc##t&>(o);	 \
      _n  = make_ndarray<double>(nx,ny);			 \
      _x  = make_ndarray<double>(nx,ny);			 \
      _xx = make_ndarray<double>(nx,ny);			 \
      _y  = make_ndarray<double>(nx,ny);			 \
      _xy = make_ndarray<double>(nx,ny);			 \
    } break

  switch(o.type()) {
  case DescEntry::Scalar:
    { const_cast<DescEntry&>(o).auto_refresh(true);
      _n  = make_ndarray<double>(1,1);
      _x  = make_ndarray<double>(1,1);
      _xx = make_ndarray<double>(1,1);
      _y  = make_ndarray<double>(1,1);
      _xy = make_ndarray<double>(1,1);
    } break;
  CASE(Scan  ,1,d.nbins());
  CASE(Prof  ,1,d.nbins());
  CASE(Prof2D,d.nybins(),d.nxbins());
  default:
    break;
  };

  memset( _n.data(),0,_n.size()*sizeof(double));
  memset( _x.data(),0,_n.size()*sizeof(double));
  memset(_xx.data(),0,_n.size()*sizeof(double));
  memset( _y.data(),0,_n.size()*sizeof(double));
  memset(_xy.data(),0,_n.size()*sizeof(double));
  _a[0]=_a[1]=_a[2]=0;

#undef CASE
}

LeastSquares::~LeastSquares()
{
}

void LeastSquares::add(EntryScalar& e, double x, double y)
{
  LeastSquaresStats s( _n[0][0],
		       _x[0][0],
		      _xx[0][0],
		       _y[0][0],
		      _xy[0][0]);

  double a[3];
  if (e.entries()>0)
    s.accum(x,y,a);
  else
    s.init(x,y,a);

  _a[0] += a[0];
  _a[1] += a[1];
  _a[2] += a[2];
  e.content(_a);
}

void LeastSquares::add(EntryScan&   e, double x, double y, double vx, double vt)
{
  unsigned current = unsigned(e.info(EntryScan::Current));
  unsigned bin = e.bin(vx);

  LeastSquaresStats s( _n[0][bin],
		       _x[0][bin],
		      _xx[0][bin],
		       _y[0][bin],
		      _xy[0][bin]);

  double a[4];
  if (bin==current)
    s.accum(x,y,a);
  else
    s.init(x,y,a);
  a[3] = vt;
  e.content(bin, a);
}

void LeastSquares::add(EntryProf&   e, double x, double y, double vx)
{
  unsigned bin = e.desc().bin(vx);
  if ( bin<e.desc().nbins() ) {
    LeastSquaresStats s( _n[0][bin],
			 _x[0][bin],
			 _xx[0][bin],
			 _y[0][bin],
			 _xy[0][bin]);

    double a[3];
    if (e.nentries(bin)>0) 
      s.accum(x,y,a);
    else
      s.init(x,y,a);
    e.content(bin, a);
  }
}

void LeastSquares::add(EntryProf2D& e, double x, double y, double vx, double vy)
{
  unsigned xbin = e.desc().xbin(vx);
  unsigned ybin = e.desc().ybin(vy);

  if (xbin < e.desc().nxbins() && 
      ybin < e.desc().nybins()) {
    LeastSquaresStats s( _n[ybin][xbin],
			 _x[ybin][xbin],
			 _xx[ybin][xbin],
			 _y[ybin][xbin],
			 _xy[ybin][xbin]);

    double a[3];
    if (e.nentries(xbin,ybin)>0) 
      s.accum(x,y,a);
    else
      s.init(x,y,a);
    e.content(xbin, ybin, a);
  }
}
