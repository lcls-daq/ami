#include "FeatureRange.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/valgnd.hh"

#include <string.h>
#include <sstream>

using namespace Ami;

static const unsigned NameSize = FeatureCache::FEATURE_NAMELEN;

FeatureRange::FeatureRange(const char* feature,
			   double lo, double hi) :
  AbsFilter(AbsFilter::FeatureRange),
  _cache(0),
  _lo   (lo),
  _hi   (hi)
{
  strncpy_val(_feature, feature, NameSize);
}

FeatureRange::FeatureRange(const char*& p, FeatureCache* f) :
  AbsFilter(AbsFilter::FeatureRange)
{
  _extract(p, _feature, NameSize);
  _extract(p, &_lo   , sizeof(_lo));
  _extract(p, &_hi   , sizeof(_hi));
  _cache = f;
  if (f)
    _index = f->lookup(_feature);
  else
    _index = -2;
}

FeatureRange::~FeatureRange()
{
}

void  FeatureRange::use   () const
{
  if (_index>=0)
    _cache->use(_index);
}

bool  FeatureRange::valid () const 
{
  bool damaged=true;
  if (_index>=0)
    _cache->cache(_index,&damaged);
  return !damaged;
}

bool  FeatureRange::accept() const 
{
  bool damaged;
  double v = -999;
  if (_index>=0)
    v = _cache->cache(_index,&damaged);
  else
    damaged = true;

  return !damaged && v>=_lo && v<=_hi;
}

void* FeatureRange::_serialize(void* p) const 
{ 
  _insert(p, _feature, NameSize );
  _insert(p, &_lo   , sizeof(_lo));
  _insert(p, &_hi   , sizeof(_hi));
  return p;
}

AbsFilter* FeatureRange::clone() const { return new FeatureRange(_feature,_lo,_hi); }

std::string FeatureRange::text() const { 
  std::ostringstream s;
  s << _lo << " LTEQ " << _feature << " LTEQ " << _hi;
  return s.str();
}
