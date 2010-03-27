#ifndef Ami_Contour_hh
#define Ami_Contour_hh

namespace Ami {

  class Contour {
  public:
    enum { MaxOrder=2 };
    Contour();
    Contour(float*, unsigned);
    Contour(const Contour&);
    ~Contour();
  public:
    float value(float) const;
    void  extremes(double  x0,double  x1,
		   double& y0,double& y1) const;
  private:
    unsigned  _n;
    float     _c[MaxOrder+1];
  };
};

#endif
