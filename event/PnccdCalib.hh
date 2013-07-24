#ifndef Ami_PnccdCalib_hh
#define Ami_PnccdCalib_hh

#include <string>

namespace Ami {
  class Entry;
  class EntryImage;
  class PnccdCalib {
  public:
    static unsigned    option_no_pedestal();
    static unsigned    option_reload_pedestal();
    static unsigned    option_correct_common_mode();
    static std::string save_pedestals(Entry*,bool corrected,bool prod);
    static void        load_pedestals(EntryImage*,bool tform);
  };
};

#endif
