#ifndef AmiQt_AxisPixelsR_hh
#define AmiQt_AxisPixelsR_hh

#include "ami/qt/AxisPixels.hh"

namespace Ami {
  namespace Qt {
    class AxisPixelsR : public AxisPixels {
    public:
      AxisPixelsR(double xlo, double xhi, int n) : AxisPixels(xlo,xhi,n) {}
      ~AxisPixelsR() {}
    public:
      int    ftick    (double p ) const;
      int    ftick_u  (double p ) const;
    };
  };
};

#endif
