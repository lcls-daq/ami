#include "FeatureRange.hh"

#include "ami/data/FeatureCache.hh"

#include <string.h>

using namespace Ami;

static const int NameSize = FeatureCache::FEATURE_NAMELEN;

FeatureRange::FeatureRange(unsigned index,
			   double lo, double hi) :
  AbsFilter(AbsFilter::FeatureRange),
  _index(index),
  _cache(0),
  _lo   (lo),
  _hi   (hi)
{
}

FeatureRange::FeatureRange(const char*& p, FeatureCache& f) :
  AbsFilter(AbsFilter::FeatureRange)
{
  _extract(p, &_index, sizeof(_index));
  _extract(p, &_lo   , sizeof(_lo));
  _extract(p, &_hi   , sizeof(_hi));
  _cache = &f;
}

FeatureRange::~FeatureRange()
{
}

bool  FeatureRange::accept() const 
{
  bool damaged;
  double v = _cache->cache(_index,&damaged);
  return !damaged && v>=_lo && v<=_hi;
}

void* FeatureRange::_serialize(void* p) const 
{ 
  _insert(p, &_index, sizeof(_index));
  _insert(p, &_lo   , sizeof(_lo));
  _insert(p, &_hi   , sizeof(_hi));
  return p;
}

AbsFilter* FeatureRange::clone() const { return new FeatureRange(_index,_lo,_hi); }
