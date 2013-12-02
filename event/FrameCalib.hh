#ifndef Ami_FrameCalib_hh
#define Ami_FrameCalib_hh

#include "ndarray/ndarray.h"

#include <string>

namespace Ami {
  class Entry;
  class EntryImage;
  class FrameCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_correct_common_mode();
    static std::string save_pedestals(Entry*,bool corrected,bool prod);
    static bool        load_pedestals(EntryImage*,unsigned);

    static int median(ndarray<const uint16_t,1> data,
		      int& iLo, int& iHi);
    static int median(ndarray<const int32_t,1> data,
		      int& iLo, int& iHi,
		      unsigned*&);
  };
};

#endif
