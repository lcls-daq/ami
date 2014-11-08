#ifndef Ami_MedianSlope_hh
#define Ami_MedianSlope_hh

#include "ami/data/LineFitAcc.hh"

#include "ndarray/ndarray.h"

namespace Ami {
  class DescEntry;
  class MedianSlope : public LineFitAcc {
  public:
    MedianSlope(const DescEntry&);
    ~MedianSlope();
  public:
    void add(EntryScalar&, double x, double y);
    void add(EntryScan&  , double x, double y, double vx, double vt);
    void add(EntryProf&  , double x, double y, double vx);
    void add(EntryProf2D&, double x, double y, double vx, double vy);
  private:
    class PrivateData;
    PrivateData* _private;
    double _a[3];
  };
};

#endif
