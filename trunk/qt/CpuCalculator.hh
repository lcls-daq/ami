#ifndef AmiQt_CpuCalculator_hh
#define AmiQt_CpuCalculator_hh

#include "ami/qt/StatCalculator.hh"

#include <time.h>

namespace Ami {
  namespace Qt {
    class CpuCalculator : public StatCalculator {
    public:
      CpuCalculator();
      ~CpuCalculator();
    public:
      void reset ();
      void update();
    private:
      timespec _last;
      double   _cputime;
    };
  };
};

#endif
