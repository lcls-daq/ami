#ifndef Ami_CmdLineTools_hh
#define Ami_CmdLineTools_hh

#include <stdint.h>

namespace Ami {
  class CmdLineTools {
  public:
    static bool parseInt   (const char* arg, int&);
    static bool parseUInt  (const char* arg, unsigned&);
    static bool parseUInt64(const char* arg, uint64_t&);
    static bool parseFloat (const char* arg, float&);
    static bool parseDouble(const char* arg, double&);
  };
};

#endif
