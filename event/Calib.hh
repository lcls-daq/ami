#ifndef Ami_Calib_hh
#define Ami_Calib_hh

#include <ndarray/ndarray.h>

#include <string>
#include <stdio.h>

namespace Pds { class DetInfo; };

namespace Ami {
  class Calib {
  public:
    static FILE* fopen     (const Pds::DetInfo&, 
                            const char* onl_calib_type,
                            const char* off_calib_type="None",
                            bool        no_cache=false,
                            bool*       offl_type=0,
			    std::string* fname=0);

    static FILE *fopen_dual(const char *path1, const char * path2, 
                            const char *description,
                            bool        no_cache=false);

    static void load_array      (ndarray<double,1>& a, 
                                 unsigned phy,
                                 const char* pfx, const char* dsc);
    static void load_integral_symm(ndarray<double,1>& a, 
				   unsigned phy,
				   const char* pfx, const char* dsc);
    static void use_offline(bool);
    static void use_test(bool);
    static bool use_test();
    static void set_experiment(const char*);
    static void set_run(int);
    static int  get_line(char** p, size_t* n, FILE* f);
    static void skip_header(FILE* f);
  };
};

#endif
