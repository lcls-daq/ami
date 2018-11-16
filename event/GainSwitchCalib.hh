#ifndef Ami_GainSwitchCalib_hh
#define Ami_GainSwitchCalib_hh

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
  class GainSwitchCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_suppress_bad_pixels();
    static unsigned    option_correct_gain       ();
    static unsigned    option_correct_common_mode();
    static unsigned    option_correct_common_mode2();
    static unsigned    option_correct_common_mode3();
    static unsigned    option_pixel_value_in_kev ();

    static unsigned    normalization_option_mask ();

    static ndarray<unsigned,2> load_array(const Pds::DetInfo&,
                                          unsigned ny,
                                          unsigned nx,
                                          unsigned def_val,
                                          bool* used_default,
                                          const char* onl_prefix,
                                          const char* off_prefix);

    static ndarray<unsigned,2> load_array(const Pds::DetInfo&,
                                          unsigned ny,
                                          unsigned nx,
                                          unsigned def_val,
                                          bool* used_default,
                                          FILE*);

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

    static ndarray<double,3> load_multi_array(const Pds::DetInfo&,
                                              unsigned nm,
                                              unsigned ny,
                                              unsigned nx,
                                              double def_val,
                                              bool* used_default,
                                              const char* onl_prefix,
                                              const char* off_prefix);

    static ndarray<double,3> load_multi_array(const Pds::DetInfo&,
                                              unsigned nm,
                                              unsigned ny,
                                              unsigned nx,
                                              double def_val,
                                              bool* used_default,
                                              FILE*);

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
                                              double def_val,
                                              bool* used_default,
                                              FILE*);
  };
};

#endif
