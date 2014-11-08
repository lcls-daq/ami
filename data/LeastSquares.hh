#ifndef Ami_LeastSquares_hh
#define Ami_LeastSquares_hh

#include "ami/data/LineFitAcc.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class DescEntry;
  class LeastSquares : public LineFitAcc {
  public:
    LeastSquares(const DescEntry&);
    ~LeastSquares();
  public:
    void add(EntryScalar&, double x, double y);
    void add(EntryScan&  , double x, double y, double vx, double vt);
    void add(EntryProf&  , double x, double y, double vx);
    void add(EntryProf2D&, double x, double y, double vx, double vy);
  private:
    ndarray<double,2> _n;
    ndarray<double,2> _x;
    ndarray<double,2> _xx;
    ndarray<double,2> _y;
    ndarray<double,2> _xy;
    double _a[3];
  };
};

#endif
