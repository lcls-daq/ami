#ifndef Ami_FccdCalib_hh
#define Ami_FccdCalib_hh

#include <string>

namespace Ami {
  class Entry;
  class FccdCalib {
  public:
    static unsigned    option_no_pedestal();
    static std::string save_pedestals(Entry*,bool corrected);
  };
};

#endif
