#ifndef Ami_FccdCalib_hh
#define Ami_FccdCalib_hh

#include <string>

namespace Ami {
  class DescEntry;
  class Entry;
  class FccdCalib {
  public:
    static unsigned    option_no_pedestal();
    static unsigned    option_reload_pedestal();
    static std::string save_pedestals(Entry*,bool corrected,bool prod);
  };
};

#endif
