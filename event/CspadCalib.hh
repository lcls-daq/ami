#ifndef Ami_CspadCalib_hh
#define Ami_CspadCalib_hh

#include <string>

namespace Ami {
  class Entry;
  class CspadCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_correct_common_mode();
    static unsigned    option_suppress_bad_pixels();
    static unsigned    option_post_integral      ();
    static std::string save_pedestals(Entry*,bool corrected,bool prod);
  };
};

#endif
