#ifndef Ami_FrameCalib_hh
#define Ami_FrameCalib_hh

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
  class FrameCalib {
  public:
    static unsigned    option_no_pedestal        ();
    static unsigned    option_reload_pedestal    ();
    static unsigned    option_suppress_bad_pixels();
    static unsigned    option_correct_gain       ();
    static unsigned    option_correct_common_mode();
    static unsigned    option_correct_common_mode2();
    static unsigned    option_correct_common_mode3();

    static std::string save_pedestals(Entry*,bool corrected,bool prod,
                                      const char* prefix="ped");
    static bool        load_pedestals(EntryImage*,unsigned,
                                      const char* prefix="ped");
    static bool        load_pedestals_all(EntryImage*,unsigned,
                                          const char* onl_prefix="ped",
                                          const char* off_prefix="pedestals");
    static bool        load_pedestals(EntryImage*,unsigned,FILE*);

    static std::string       save(ndarray<const double,3>,
                                  const DescImage&,
                                  bool prod,
                                  const char* prefix="gain");
    static ndarray<double,3> load(const DescImage&,
                                  const char* prefix="gain");
    static ndarray<double,3> load(const DescImage&,FILE*);

    static ndarray<unsigned,2> load_array(const DescImage&,
                                          const char* prefix="gain");
    static ndarray<double,2>   load_darray(const DescImage&,
                                           const char* prefix="gain");
    static ndarray<double,2>   load_darray(const DescImage&,
                                           const char* prefix,
                                           const char* offl_type);

    static ndarray<unsigned,2>   load_array (FILE*);
    static ndarray<double  ,2>   load_darray(FILE*);
    
    static int median(ndarray<const uint16_t,1> data,
                      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const uint32_t,1> data,
                      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const uint32_t,2> data,
                      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const int32_t,1> data,
                      int& iLo, int& iHi,
                      unsigned*&);

    static int median(ndarray<const uint16_t,1> data,
                      ndarray<const uint16_t,1> status,
                      unsigned& iLo, unsigned& iHi);
    static int median(ndarray<const uint32_t,1> data,
                      ndarray<const uint32_t,1> status,
                      unsigned& iLo, unsigned& iHi);

    static int frameNoise(ndarray<const uint32_t,1> data,
                          ndarray<const uint32_t,1> status,
                          unsigned off);
    static double frameNoise(ndarray<const uint32_t,2> data,
                             ndarray<const uint32_t,2> status,
                             unsigned off);
  };
};

#endif
