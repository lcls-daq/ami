#ifndef Ami_FeatureCache_hh
#define Ami_FeatureCache_hh

#include "ami/service/Semaphore.hh"

#include <stdint.h>
#include <vector>
#include <string>

namespace Ami {

  enum ScalarSet { PreAnalysis, PostAnalysis, NumberOfSets };

  class FeatureCache {
  public:
    enum { FEATURE_NAMELEN=64 };
    FeatureCache();
    ~FeatureCache();
  public:
    void     clear ();
    unsigned add   (const std::string&);
    unsigned add   (const char* name);
    void     add   (const FeatureCache&);
    int      lookup(const char* name) const;
    void     rename(unsigned, const char*);
    void     start ();
    bool     update();
  public:
    unsigned    entries() const;
    const std::vector<std::string>& names() const;
    double      cache  (int index, bool* damaged=0) const;
    void        cache  (int index, double, bool damaged=false);
    void        cache  (const FeatureCache&);
    void        dump   () const;
  public:
    void        clear_used();
    void        use    (int index);
    void        use    (const FeatureCache&);
    bool        used   (int index) const;
  public:
    char*  serialize(int& len) const;
  private:
    std::vector<std::string> _names;
    std::vector<double>      _cache;
    std::vector<uint32_t>    _damaged;
    std::vector<uint32_t>    _used;
    bool                     _update;
    mutable Semaphore        _sem;
  };
};

#endif
