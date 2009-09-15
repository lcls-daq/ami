#ifndef Ami_FeatureCache_hh
#define Ami_FeatureCache_hh

#include <stdint.h>

namespace Ami {
  class FeatureCache {
  public:
    enum { FEATURE_NAMELEN=32 };
    FeatureCache();
    ~FeatureCache();
  public:
    void     clear();
    unsigned add(const char* name);
  public:
    unsigned    entries() const;
    const char* names  () const;
    double      cache  (unsigned index, bool* damaged=0) const;
    void        cache  (unsigned index, double, bool damaged=0);
  private:
    unsigned  _entries;
    unsigned  _max_entries;
    char*     _names;
    double*   _cache;
    uint32_t* _damaged;
  };
};

#endif
