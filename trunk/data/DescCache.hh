#ifndef Pds_DescCache_hh
#define Pds_DescCache_hh

#include "ami/data/DescEntry.hh"
#include "ami/data/FeatureCache.hh"

namespace Ami {
  class DescCache : public DescEntry {
  public:
    DescCache(const char* name, 
              const char* ytitle,
              ScalarSet   set);
  public:
    ScalarSet set() const { return ScalarSet(_set); }
  private:
    uint32_t _set;
    uint32_t _reserved;
  };
};

#endif
