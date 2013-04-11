#ifndef Ami_Calib_hh
#define Ami_Calib_hh

#include <stdio.h>

namespace Ami {
  class Calib {
  public:
    static FILE *fopen_dual(const char *path1, const char * path2, const char *description);
  };
};

#endif
