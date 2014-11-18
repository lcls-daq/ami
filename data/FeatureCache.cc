#include "FeatureCache.hh"
#include "ami/data/valgnd.hh"

#include <string.h>
#include <stdio.h>

//#define DBUG

using namespace Ami;

FeatureCache::FeatureCache() :
  _update(false),
  _sem   (Semaphore::FULL)
{
}

FeatureCache::~FeatureCache()
{
}

void     FeatureCache::clear()
{
  _sem.take();
  _names.clear();
  _cache.clear();
  _damaged.clear();
  _sem.give();
}

unsigned FeatureCache::add(const std::string& name)
{ return add(name.c_str()); }

unsigned FeatureCache::add(const char* name)
{
  std::string sname(name);
  
  for(unsigned k=0; k<_names.size(); k++)
    if (sname==_names[k])
      return k;

  _sem.take();
  _update=true;
  _names.push_back(sname);
  unsigned len = _names.size();
  unsigned index = len - 1;
  _cache  .resize(len);
  _damaged.resize((len+0x1f)>>5);
  _damaged[index>>5] |= 1<<(index&0x1f);
  _used.resize((len+0x1f)>>5);
  _sem.give();

#ifdef DBUG
  printf("FeatureCache::add[%p] [%d] %s\n",this,index,name);
#endif

  return index;
}

void FeatureCache::add(const FeatureCache& c)
{
  for(unsigned k=0; k<c._names.size(); k++)
    add(c._names[k]);
}

int         FeatureCache::lookup(const char* name) const
{
  std::string sname(name);

  for(unsigned k=0; k<_names.size(); k++)
    if (sname==_names[k])
      return int(k);

  return -1;
}

void        FeatureCache::rename(unsigned index, const char* name)
{
  _names[index] = std::string(name);
}

unsigned    FeatureCache::entries() const { return _names.size(); }
const std::vector<std::string>& FeatureCache::names  () const { return _names; }
double      FeatureCache::cache  (int index, bool* damaged) const 
{
  if (damaged) *damaged = (index<0) || ((_damaged[index>>5]>>(index&0x1f)) & 1);
  return index>=0 ? _cache[index] : 0;
}

void        FeatureCache::cache  (int index, double v, bool damaged)
{
  if (index>=0) {
    _cache[index] = v;
    uint32_t mask = 1<<(index&0x1f);
    if (damaged)
      _damaged[index>>5] |= mask;
    else
      _damaged[index>>5] &= ~mask;
#ifdef DBUG
    printf("FeatureCache::%s[%p] [%d] %s\n",damaged?"dmg":"val",this,index,_names[index].c_str());
#endif
  }
}

void        FeatureCache::cache  (const FeatureCache& c)
{
  unsigned j=0;
  for(unsigned i=0; i<c._cache.size(); i++) {
    const std::string& name = c._names[i];
    for(unsigned k=j; k<_cache.size(); k++)
      if (_names[k]==name) {
        cache(j=k, c._cache[i], (c._damaged[i>>5] & (1<<(i&0x1f))));
	j++;
        break;
      }
  }
}

void   FeatureCache::clear_used()
{
  for(unsigned k=0; k<_used.size(); k++)
    _used[k] = 0;
#ifdef DBUG
  printf("FeatureCache[%p]::clear_used()\n",this);
#endif
}

void   FeatureCache::use(int index)
{
  _used[index>>5] |= 1<<(index&0x1f);
#ifdef DBUG
  printf("FeatureCache[%p]::use [%d][%s] : %08x\n",
	 this,index,_names[index].c_str(),_used[index>>5]);
#endif
}

void   FeatureCache::use (const FeatureCache& c)
{
  unsigned j=0;
  for(unsigned i=0; i<c._names.size(); i++) {
    if (c.used(i)) {
      const std::string& name = c._names[i];
      for(unsigned k=j; k<_names.size(); k++)
	if (_names[k]==name) {
	  use(j=k);
	  j++;
	  break;
	}
    }
  }
}

bool   FeatureCache::used(int index) const
{
  bool v = index>=0 && (_used[index>>5] & (1<<(index&0x1f)));
#ifdef DBUG
  printf("FeatureCache[%p]::used[%d][%s=%c] : %08x\n",
	 this,index,_names[index].c_str(),v?'T':'F',_used[index>>5]);
#endif
  return v;
}

char*  FeatureCache::serialize(int& len) const
{
  _sem.take();
  len = _names.size()*FEATURE_NAMELEN;
  char* result = new char[len];
  char* p = result;
  for(unsigned k=0; k<_names.size(); k++, p+=FEATURE_NAMELEN) {
    strncpy_val(p, _names[k].c_str(), FEATURE_NAMELEN);
  }
  _sem.give();
  return result;
}

bool   FeatureCache::update()
{
  bool upd(_update);
  _update=false;
  return upd;
}

void   FeatureCache::dump() const
{
  printf("FeatureCache entries %d\n",(int) _names.size());
  for(unsigned k=0; k<_names.size(); k++) {
    printf("  %s [%f] %c\n",_names[k].c_str(), _cache[k], 
	   ((_damaged[k>>5]>>(k&0x1f)) & 1) ? 'D':' ');
  }
}

void   FeatureCache::start()
{
  for(unsigned k=0; k<_damaged.size(); k++)
    _damaged[k] = uint32_t(-1UL);
}
