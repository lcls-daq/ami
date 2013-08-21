#ifndef Ami_FrameCalib_hh
#define Ami_FrameCalib_hh

#include <string>

namespace Ami {
  class Entry;
  class FrameCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_correct_common_mode();
    static std::string save_pedestals(Entry*,bool corrected,bool prod);
  };
};

#endif
