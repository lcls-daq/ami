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
  _names.clear();
}

void               FeatureRegistry::insert(const char* names,
					   unsigned n)
{
  for(unsigned k=0; k<n; k++) {
    _names << QString(names);
    names += NameSize;
  }
  emit changed();
}

const QStringList& FeatureRegistry::names () const
{
  return _names;
}

int FeatureRegistry::index(const QString& name) const
{
  return _names.indexOf(name);
}

FeatureRegistry::FeatureRegistry() {}

FeatureRegistry::~FeatureRegistry() {}
