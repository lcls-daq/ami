#ifndef Ami_UxiCalib_hh
#define Ami_UxiCalib_hh

#include "ndarray/ndarray.h"

#include <stdint.h>
#include <string>

namespace Pds {
  class DetInfo;
}

namespace Ami {
  class Entry;
  class EntryImage;
  class DescImage;
  class UxiCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_suppress_bad_pixels();
    static unsigned    option_correct_gain       ();
    static unsigned    option_correct_common_mode();

    static ndarray<unsigned,3> load_array(const Pds::DetInfo&,
                                          unsigned nz,
                                          unsigned ny,
                                          unsigned nx,
                                          unsigned def_val,
                                          bool* used_default,
                                          const char* onl_prefix,
                                          const char* off_prefix);

    static ndarray<unsigned,3> load_array(const Pds::DetInfo&,
                                          unsigned nz,
                                          unsigned ny,
                                          unsigned nx,
                                          unsigned def_val,
                                          bool* used_default,
                                          FILE*);
  };
};

#endif
