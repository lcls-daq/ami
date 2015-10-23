#ifndef Ami_CspadAlignment_hh
#define Ami_CspadAlignment_hh

#include "ami/data/DescImage.hh"
#include <stdio.h>

//
//  Class to convert optical alignment measurements for display
//  Optical alignment coordinates assume device in quadrant 1 position
//

namespace Ami {
  namespace Cspad {

    struct TwoByOneAlignment {
      struct { double x,y; } _corner[4];
      struct { double x,y; } _pad;
      Rotation               _rot;
    };

    class QuadAlignment {
    public:
      //      QuadAlignment();
    public:
      struct TwoByOneAlignment _twobyone[8];
    public:
      static QuadAlignment* load   (FILE*, bool offline=false);
      static QuadAlignment* load2x2(FILE*, bool offline=false);
    };
  }
}

#endif
