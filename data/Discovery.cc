#include "Discovery.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/DescEntry.hh"

using namespace Ami;

static const unsigned FEATURE_NAMELEN = FeatureCache::FEATURE_NAMELEN;

DiscoveryTx::DiscoveryTx(const std::vector<FeatureCache*>& f,
			 const Cds&          cds) :
  _features(f),
  _cds     (cds)
{
}

DiscoveryTx::~DiscoveryTx() 
{
  for(unsigned i=0; i<_cache.size(); i++)
    delete[] _cache[i];
}

unsigned DiscoveryTx::niovs() const
{
  return _cds.description()+_features.size()+1;
}

void DiscoveryTx::serialize(iovec* iov)
{
  iov[0].iov_base = &_header[0];
  iov[0].iov_len  = (NumberOfSets+1)*sizeof(uint32_t);

  _header[NumberOfSets] = 1;

  for(unsigned i=0; i<_features.size(); i++) {

    _header[i] = _features[i]->entries();
    
    int   len;
    char* buff = _features[i]->serialize(len);
    _cache.push_back(buff);

    iov[i+1].iov_base = buff;
    iov[i+1].iov_len  = len;
  }
  _cds.description(iov+_features.size()+1);
}


DiscoveryRx::DiscoveryRx(const char* msg, unsigned size) :
  _p(msg), _size(size) 
{
}

DiscoveryRx::~DiscoveryRx() {}

std::vector<std::string> DiscoveryRx::features(ScalarSet set) const
{
  const char* p = _p + sizeof(uint32_t)*(NumberOfSets+1);
  for(unsigned i=0; i<unsigned(set); i++)
    p += reinterpret_cast<const uint32_t*>(_p)[i]*FEATURE_NAMELEN;

  unsigned n = reinterpret_cast<const uint32_t*>(_p)[set];

  std::vector<std::string> v;
  v.reserve(n);
  for(unsigned i=0; i<n; i++, p+=FEATURE_NAMELEN) {
    v.push_back(std::string(p));
  }
    
  return v;
}

const Desc*      DiscoveryRx::title_desc() const
{
  const char* p = _p + sizeof(uint32_t)*(NumberOfSets+1);
  for(unsigned i=0; i<NumberOfSets; i++)
    p += reinterpret_cast<const uint32_t*>(_p)[i]*FEATURE_NAMELEN;
  
  return reinterpret_cast<const Desc*>(p);
}

const DescEntry* DiscoveryRx::entries() const
{
  return reinterpret_cast<const DescEntry*>(title_desc()+1);
}

const DescEntry* DiscoveryRx::end() const
{
  return reinterpret_cast<const DescEntry*>(_p+_size);
}

const DescEntry* DiscoveryRx::entry(const char* name) const
{
  for(  const DescEntry* e = entries(); e < end(); 
        e = reinterpret_cast<const DescEntry*>
          (reinterpret_cast<const char*>(e) + e->size()))
    if (strcmp(e->name(),name)==0)
      return e;
  return 0;
}

unsigned DiscoveryRx::nsources() const
{
  return reinterpret_cast<const uint32_t*>(_p)[NumberOfSets];
}

void DiscoveryRx::nsources(unsigned n) 
{
  char* p = const_cast<char*>(_p);
  reinterpret_cast<uint32_t*>(p)[NumberOfSets] = n;
}
