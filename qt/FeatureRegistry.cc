#include "FeatureRegistry.hh"

#include "ami/data/FeatureCache.hh"

static int NameSize = Ami::FeatureCache::FEATURE_NAMELEN;

using namespace Ami::Qt;

static FeatureRegistry* _instance=0;

FeatureRegistry& FeatureRegistry::instance() 
{
  if (!_instance) _instance = new FeatureRegistry;
  return *_instance;
}

void               FeatureRegistry::clear ()
{
  _sem.take();
  _names.clear();
  _sem.give();
}

void               FeatureRegistry::insert(const char* names,
					   unsigned n)
{
  _sem.take();
  for(unsigned k=0; k<n; k++) {
    _names << QString(names);
    names += NameSize;
  }
  _sem.give();
  emit changed();
}

QStringList FeatureRegistry::names () const
{
  const_cast<FeatureRegistry*>(this)->_sem.take();
  QStringList l(_names);
  const_cast<FeatureRegistry*>(this)->_sem.give();
  return l;
}

int FeatureRegistry::index(const QString& name) const
{
  const_cast<FeatureRegistry*>(this)->_sem.take();
  int v = _names.indexOf(name);
  const_cast<FeatureRegistry*>(this)->_sem.give();
  return v;
}

FeatureRegistry::FeatureRegistry() : _sem(Ami::Semaphore::FULL) {}

FeatureRegistry::~FeatureRegistry() {}
