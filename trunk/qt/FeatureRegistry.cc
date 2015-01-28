#include "FeatureRegistry.hh"
#include "Path.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/Discovery.hh"
#include "ami/qt/SharedData.hh"

#include <stdio.h>

using namespace Ami::Qt;

static FeatureRegistry* _instance[Ami::NumberOfSets] = {0,0};

FeatureRegistry& FeatureRegistry::instance(Ami::ScalarSet t) 
{
  if (!_instance[t]) _instance[t] = new FeatureRegistry;
  return *(_instance[t]);
}

void               FeatureRegistry::insert(const std::vector<std::string>& rx)
{
  printf("FeatureRegistry insert %zd entries\n",rx.size());

  _sem.take();
  _names.clear();
  _help .clear();
  for(unsigned i=0; i<rx.size(); i++) {
    _names << QString(rx[i].c_str());
    _help  << QString();
  }

  FILE* f = Path::helpFile();
  if (f) {
    char* line = new char[256];
    while( fgets(line, 256, f) ) {
      if (line[0]=='#') continue;  // comment field
      static const char* delim = " \t";
      char* p = line;
      strsep(&p,delim);
      if (p) {
	p += strspn(p,delim);
	*strchrnul(p,'\n') = 0;
	QString name(line);
	int index = _names.indexOf(name);
	if (index >= 0)
	  _help[index] = QString(p);
      }
    }
    delete[] line;
    fclose(f);
  }

  _sem.give();
  emit changed();
}

QStringList FeatureRegistry::names () const
{
  _sem.take();
  QStringList n(_names);
  _sem.give();
  return n;
}

const QStringList& FeatureRegistry::help() const
{
  return _help;
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

QString FeatureRegistry::validate_name(const QString& n) const
{
  QString m;
  if (this == _instance[PostAnalysis] && !n.startsWith("Post:"))
    m = QString("Post:%1").arg(n);
  else
    m = n;

  QString v(m);
  unsigned j=0;
  while (_names.contains(v))
    v = QString("%1_%2").arg(m).arg(++j);
  return v;
}

QString FeatureRegistry::find_name(const QString& n) const
{
  QString m;
  if (this == _instance[PostAnalysis] && !n.startsWith("Post:"))
    m = QString("Post:%1").arg(n);
  else
    m = n;

  QString v(m);
  return (_names.contains(v)) ? v : QString();
}

void         FeatureRegistry::share        (const QString& name, SharedData* s)
{
  std::map<QString,SharedData*>::iterator it=_shared.find(name);
  if (it!=_shared.end())
    printf("FeatureRegistry::shared inserting duplicate entry [%s][%p->%p]!\n",
           qPrintable(name), _shared[name], s);

  _shared[name] = s;
}

void         FeatureRegistry::remove       (const QString& name)
{
  std::map<QString,SharedData*>::iterator it=_shared.find(name);
  if (it!=_shared.end()) {
    it->second->resign();
    _shared.erase(it);
    _names.removeAll(name);
  }
}
