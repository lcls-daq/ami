#ifndef Ami_FrameCalib_hh
#define Ami_FrameCalib_hh

#include "ndarray/ndarray.h"

#include <string>

namespace Ami {
  class Entry;
  class EntryImage;
  class DescImage;
  class FrameCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_correct_gain       ();
    static unsigned    option_correct_common_mode();
    static unsigned    option_correct_common_mode2();

    static std::string save_pedestals(Entry*,bool corrected,bool prod,
				      const char* prefix="ped");
    static bool        load_pedestals(EntryImage*,unsigned,
				      const char* prefix="ped");

    static std::string       save(ndarray<const double,3>,
				  const DescImage&,
				  bool prod,
				  const char* prefix="gain");
    static ndarray<double,3> load(const DescImage&,
				  const char* prefix="gain");
    
    static int median(ndarray<const uint16_t,1> data,
		      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const uint32_t,1> data,
		      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const int32_t,1> data,
		      int& iLo, int& iHi,
		      unsigned*&);
  };
};

#endif
