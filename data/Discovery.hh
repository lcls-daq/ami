#ifndef Ami_Discovery_hh
#define Ami_Discovery_hh

#include "ami/data/DescCache.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace Ami {
  class FeatureCache;
  class Cds;
  class Desc;
  class DescEntry;

  enum { SourceInfo=0, TagInfo, InfoSize };

  class DiscoveryTx {
  public:
    DiscoveryTx(const std::vector<FeatureCache*>&, const Cds&);
    ~DiscoveryTx();
  public:
    unsigned niovs() const;
    void     serialize(iovec*);
  private:
    const std::vector<FeatureCache*>& _features;
    const Cds&          _cds;
    mutable uint32_t    _header[NumberOfSets+InfoSize];
    std::vector<const char*> _cache;
  };

  class DiscoveryRx {
  public:
    DiscoveryRx(const char* msg, unsigned size);
    ~DiscoveryRx();
  public:
    std::vector<std::string> features(ScalarSet =PreAnalysis) const;
    
    const Desc*      title_desc() const;
    const DescEntry* entries() const;
    const DescEntry* end    () const;
    const DescEntry* entry  (const char*) const;
  public:
    const char* payload() const { return _p; }
    unsigned    payload_size() const { return _size; }
  public:
    unsigned    nsources() const;
    void        nsources(unsigned);
    unsigned    tag     () const;
    void        tag     (unsigned);
  public:
    void        dump() const;
  private:
    const char* _p;
    uint32_t    _size;
  };
};
#endif
