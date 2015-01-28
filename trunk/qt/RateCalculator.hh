#ifndef AmiQt_RateCalculator_hh
#define AmiQt_RateCalculator_hh

#include "ami/qt/StatCalculator.hh"

#include <time.h>

namespace Ami {
  namespace Qt {
    class RateCalculator : public StatCalculator {
    public:
      RateCalculator();
      ~RateCalculator();
    public:
      void reset ();
      void update();
    private:
      timespec _last;
      double   _entries;
    };
  };
};

#endif
