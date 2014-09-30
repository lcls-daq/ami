#include "ami/data/EntryCache.hh"

#include "ami/data/FeatureCache.hh"

#include <stdio.h>

using namespace Ami;

EntryCache::~EntryCache() {}

EntryCache::EntryCache(const DescCache& desc, FeatureCache* cache) :
  _desc(desc),
  _cache(cache)
{
  allocate(0);
  if (_cache)
    _index = cache->add(desc.ytitle());
  else {
    printf("EntryCache ctor [%s] without cache reference\n",
	   _desc.name());
    //  This is allowed on the client-side
    //    abort();
  }
}

void EntryCache::set(double y, bool damaged)
{
  if (_cache) _cache->cache(_index,y,damaged);
}
