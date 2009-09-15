#ifndef Ami_FeatureRange_hh
#define Ami_FeatureRange_hh

#include "AbsFilter.hh"

namespace Ami {
  class FeatureCache;
  class FeatureRange : public AbsFilter {
  public:
    FeatureRange(unsigned i, double lo, double hi);
    FeatureRange(const char*&, FeatureCache&);
    ~FeatureRange();
  public:
    bool  accept() const;
    AbsFilter* clone() const;
  private:
    void* _serialize(void*) const;
  private:
    unsigned      _index;
    FeatureCache* _cache;
    double        _lo;
    double        _hi;
  };
};

#endif
