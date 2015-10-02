#ifndef Ami_PnccdCalib_hh
#define Ami_PnccdCalib_hh

#include "ami/data/DescImage.hh"
#include "ndarray/ndarray.h"

#include <string>

namespace Ami {
  class Entry;
  class EntryImage;
  class PnccdCalib {
  public:
    static unsigned    option_no_pedestal();
    static unsigned    option_reload_pedestal();
    static unsigned    option_correct_common_mode();
    static unsigned    option_rotate();
    static std::string save_pedestals(const Entry*,bool rotated,bool prod);
    static std::string save_cmth     (const Entry*,bool rotated,bool prod,double factor=1);
    static ndarray<double,2> load_pedestals(const DescImage&,Rotation, bool no_cache);
    static ndarray<double,2> load_cmth     (const DescImage&,Rotation, bool no_cache);
  };
};

#endif
