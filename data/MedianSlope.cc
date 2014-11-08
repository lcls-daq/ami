#include "ami/data/MedianSlope.hh"

#include "ami/data/Median.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"

#include <list>
#include <set>

#include <float.h>

namespace Ami {

  typedef std::pair<double,double> Point;

  class Slope {
  public:
    Slope(double wgt, double slope) : _wgt(wgt), _slope(slope) {}
    ~Slope() {}
  public:
    bool operator<(const Slope& o) const { return _slope < o._slope; }
  public:
    mutable double _wgt;
    double _slope;
  };

  class MedianSlopeStats {
  public:
    MedianSlopeStats( std::list<Point>& points,
		      Median          & slopes) :
      _points(points), _slopes(slopes) {}
  public:
    void init (double x, double y, double* a) {
      _points.clear();
      _points.push_back(Point(x,y));
      _slopes = Median();
      
      a[0] =DBL_MIN;
      a[1] =0;
      a[2] =0;
    }
    void accum(double x, double y, double* a) {
      for(std::list<Point>::const_iterator it=_points.begin();
	  it!=_points.end(); it++) {
	double wt    = fabs(x-it->first);
	if (wt>0)
	  _slopes.accum((y-it->second)/(x-it->first),wt);
      }
      _points.push_back(Point(x,y));

      if (_slopes.weight()==0) {
	a[0] = DBL_MIN;
	a[1] = 0;
	a[2] = 0;
	return;
      }

      double slope = _slopes.median();
      a[0] = _slopes.weight();
      a[1] = a[0]*slope;

      Median _intercept;
      for(std::list<Point>::const_iterator it=_points.begin();
	  it!=_points.end(); it++)
	_intercept.accum( it->second - it->first*slope );

      a[2] = a[0]*_intercept.median();
    }
  private:
    std::list<Point>& _points;
    Median          & _slopes;
  };

  class MedianSlope::PrivateData {
  public:
    PrivateData(const DescEntry& o) {
#define CASE(t,nx,ny)						\
      case DescEntry::t:					\
	{ const Desc##t& d = static_cast<const Desc##t&>(o);	\
	  _points  = make_ndarray< std::list<Point> >(nx,ny);	\
	  _slopes  = make_ndarray< Median           >(nx,ny);	\
	} break

      switch(o.type()) {
      case DescEntry::Scalar:
	{ const_cast<DescEntry&>(o).auto_refresh(true);
	  _points  = make_ndarray< std::list<Point> >(1,1);
	  _slopes  = make_ndarray< Median           >(1,1);
	} break;
	CASE(Scan  ,1,d.nbins());
	CASE(Prof  ,1,d.nbins());
	CASE(Prof2D,d.nybins(),d.nxbins());
      default:
	break;
      };

#undef CASE
    }
  public:
    MedianSlopeStats stats(unsigned xbin, unsigned ybin) {
      return MedianSlopeStats(_points[xbin][ybin],
			      _slopes[xbin][ybin]);
    }
  private:
    ndarray<std::list<Point>,2> _points;
    ndarray<Median          ,2> _slopes;
  };
  
};

using namespace Ami;

MedianSlope::MedianSlope(const DescEntry& o) :
  _private(new PrivateData(o))
{
  _a[0]=_a[1]=_a[2]=0;
}

MedianSlope::~MedianSlope()
{
  delete _private;
}

void MedianSlope::add(EntryScalar& e, double x, double y)
{
  MedianSlopeStats s(_private->stats(0,0));

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

void MedianSlope::add(EntryScan&   e, double x, double y, double vx, double vt)
{
  unsigned current = unsigned(e.info(EntryScan::Current));
  unsigned bin = e.bin(vx);

  MedianSlopeStats s(_private->stats(0,bin));

  double a[4];
  if (bin==current)
    s.accum(x,y,a);
  else
    s.init(x,y,a);
  a[3] = vt;
  e.content(bin, a);
}

void MedianSlope::add(EntryProf&   e, double x, double y, double vx)
{
  unsigned bin = e.desc().bin(vx);
  if (bin < e.desc().nbins()) {
    MedianSlopeStats s(_private->stats(0,bin));

    double a[3];
    if (e.nentries(bin)>0) 
      s.accum(x,y,a);
    else
      s.init(x,y,a);
    e.content(bin, a);
  }
}

void MedianSlope::add(EntryProf2D& e, double x, double y, double vx, double vy)
{
  unsigned xbin = e.desc().xbin(vx);
  unsigned ybin = e.desc().ybin(vy);
  if (xbin < e.desc().nxbins() &&
      ybin < e.desc().nybins()) {
    MedianSlopeStats s(_private->stats(ybin,xbin));

    double a[3];
    if (e.nentries(xbin,ybin)>0) 
      s.accum(x,y,a);
    else
      s.init(x,y,a);
    e.content(xbin, ybin, a);
  }
}
