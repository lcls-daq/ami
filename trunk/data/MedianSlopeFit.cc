#include "ami/data/MedianSlopeFit.hh"

#include "ami/data/Median.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"

#include <list>
#include <set>

namespace Ami {

  class Point {
  public:
    Point(double x,double y,double w) : first(x), second(y), third(w) {}
  public:
    double first, second, third;
  };

  class MedianSlopeFitStats {
  public:
    MedianSlopeFitStats() {}
  public:
    void accum(double x, double y, double w) {
      for(std::list<Point>::const_iterator it=_points.begin();
	  it!=_points.end(); it++) {
	double wt    = w*fabs(x-it->first);
	if (wt>0)
	  _slope.accum((y-it->second)/(x-it->first),wt);
      }
      _points.push_back(Point(x,y,w));
    }
    void fit(double& slope, double& intercept) const
    {
      slope = _slope.median();

      Median _intercept;
      for(std::list<Point>::const_iterator it=_points.begin();
	  it!=_points.end(); it++)
	_intercept.accum(it->second - it->first*slope,it->third);

      intercept=_intercept.median();
    }
  private:
    std::list<Point> _points;
    Median           _slope;
  };
};

using namespace Ami;

MedianSlopeFit::MedianSlopeFit()
{
}

MedianSlopeFit::~MedianSlopeFit()
{
}

void MedianSlopeFit::fit(double& slope, double& intercept, 
			 double& x0   , double& x1       ,
			 const EntryProf& e)
{
  MedianSlopeFitStats s;
  const DescProf& d = e.desc();
  double dx = (d.xup()-d.xlow())/double(d.nbins());
  double xo = d.xlow()+0.5*dx;
  x0 = d.xup();
  for(unsigned i=0; i<d.nbins(); i++)
    if (e.nentries(i)>0) {
      double x = xo+double(i)*dx;
      s.accum( x, e.ymean(i), e.nentries(i) );
      x1 = x;
      if (x<x0) x0=x;
    }
  s.fit(slope,intercept);
}

void MedianSlopeFit::fit(double& slope, double& intercept, 
			 double& x0   , double& x1       ,
			 const EntryScan& e)
{
  MedianSlopeFitStats s;
  const DescScan& d = e.desc();
  x0=x1=e.xbin(0);
  for(unsigned i=0; i<d.nbins(); i++) {
    double x = e.xbin(i);
    s.accum( x, e.ymean(i), e.nentries(i) );
    if (x<x0) x0=x;
    if (x>x1) x1=x;
  }
  s.fit(slope,intercept);
}

