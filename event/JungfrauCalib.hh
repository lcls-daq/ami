#ifndef Ami_JungfrauCalib_hh
#define Ami_JungfrauCalib_hh

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
  class JungfrauCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_suppress_bad_pixels();
    static unsigned    option_correct_gain       ();
    static unsigned    option_correct_common_mode();
    static unsigned    option_pixel_value_in_kev ();

    static unsigned    normalization_option_mask ();

    static ndarray<double,4> load_multi_array(const Pds::DetInfo&,
                                              unsigned nm,
                                              unsigned nz,
                                              unsigned ny,
                                              unsigned nx,
                                              double def_val,
                                              bool* used_default,
                                              const char* onl_prefix,
                                              const char* off_prefix);

    static ndarray<double,4> load_multi_array(const Pds::DetInfo&,
                                              unsigned nm,
                                              unsigned nz,
                                              unsigned ny,
                                              unsigned nx,
                                              double used_default,
                                              bool* def_applied,
                                              FILE*);
  };
};

#endif
