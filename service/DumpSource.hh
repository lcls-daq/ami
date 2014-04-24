#ifndef Ami_DumpSource_hh
#define Ami_DumpSource_hh

#include <string>

namespace Ami {
  class DumpSource {
  public:
    virtual ~DumpSource() {}
  public:
    virtual std::string dump() const = 0;
  };
};

#endif
