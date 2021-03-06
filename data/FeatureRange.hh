#ifndef Ami_FeatureRange_hh
#define Ami_FeatureRange_hh

#include "AbsFilter.hh"
#include "ami/data/FeatureCache.hh"

namespace Ami {
  class FeatureCache;
  class FeatureRange : public AbsFilter {
  public:
    FeatureRange(const char* feature, double lo, double hi);
    FeatureRange(const char*&, FeatureCache*);
    ~FeatureRange();
  public:
    void  use   () const;
    bool  valid () const;
    bool  accept() const;
    AbsFilter* clone() const;
    std::string text() const;
  public:
    const char* feature() const { return _feature; }
  private:
    void* _serialize(void*) const;
  private:
    char          _feature[FeatureCache::FEATURE_NAMELEN];
    FeatureCache* _cache;
    double        _lo;
    double        _hi;
    int           _index;
  };
};

#endif
