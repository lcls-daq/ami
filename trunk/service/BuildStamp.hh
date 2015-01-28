#ifndef Ami_BuildStamp_hh
#define Ami_BuildStamp_hh

namespace Ami {
  class BuildStamp {
  public:
    static const char* tag ();
    static const char* time();
  };
};

#endif
