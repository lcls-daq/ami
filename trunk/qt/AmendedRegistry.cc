#include "ami/qt/AmendedRegistry.hh"

#include <stdio.h>

using namespace Ami::Qt;

AmendedRegistry::AmendedRegistry(FeatureRegistry&   base,
				 const QStringList& additions) :
  _base     (base),
  _additions(additions)
{
  change();
  connect(&base, SIGNAL(changed()), this, SLOT(change()));
}

void AmendedRegistry::translate(char* p) const
{
  QString v(p);
  for(unsigned i=0; int(i)<_additions.size(); i++) {
    v.replace( QString("_%1").arg(_additions[i]), QString("[%1]").arg(i) );
  }
  strcpy(p, v.toAscii().constData());
}

void AmendedRegistry::change()
{
  unsigned n = _names.size();

  _names = _base.names();
  for(int i=0; i<_additions.size(); i++) {
    _names << QString("_%1").arg(_additions[i]);
    _help  << QString();
  }
  
  printf("AmendedRegistry names [%d] -> [%d]\n",
	 n, _names.size());

  emit changed();
}

