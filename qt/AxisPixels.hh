#ifndef AmiQt_AxisPixels_hh
#define AmiQt_AxisPixels_hh

#include "ami/qt/AxisBins.hh"

namespace Ami {
  namespace Qt {
    class AxisPixels : public AxisBins {
    public:
      AxisPixels(double xlo, double xhi, int n) : AxisBins(xlo,xhi,n) {}
      ~AxisPixels() {}
    public:
      int    tick    (double p ) const;
      int    tick_u  (double p ) const;
    };
  };
};

#endif
