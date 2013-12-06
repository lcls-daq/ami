#ifndef Ami_Calib_hh
#define Ami_Calib_hh

#include <ndarray/ndarray.h>

#include <stdio.h>

namespace Ami {
  class Calib {
  public:
    static FILE *fopen_dual(const char *path1, const char * path2, const char *description);
    static void load_array      (ndarray<double,1>& a, 
                                 unsigned phy,
                                 const char* pfx, const char* dsc);
  };
};

#endif
