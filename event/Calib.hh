#ifndef Ami_Calib_hh
#define Ami_Calib_hh

#include <stdio.h>

namespace Ami {
  class Calib {
  public:
    static FILE *fopen_dual(char *path1,char * path2, char *description);
  };
};

#endif
