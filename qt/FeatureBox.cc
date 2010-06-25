#include "FeatureBox.hh"

#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/QtPersistent.hh"

using namespace Ami::Qt;

FeatureBox::FeatureBox() :
  QHComboBox(FeatureRegistry::instance().names(),
	     FeatureRegistry::instance().help())
{
  connect(&FeatureRegistry::instance(), SIGNAL(changed()), this, SLOT(change_features()));
  connect(this, SIGNAL(activated(const QString&)), this, SLOT(set_entry(const QString&)));
}

FeatureBox::~FeatureBox()
{
}

void FeatureBox::save(char*& p) const
{
  QtPersistent::insert(p, _entry);
}

void FeatureBox::load(const char*& p)
{
  _entry = QtPersistent::extract_s(p);
  _seek();
}

const QString& FeatureBox::entry() const { return _entry; }

void FeatureBox::change_features()
{
  clear();
  addItems(FeatureRegistry::instance().names());
  _seek();
}

void FeatureBox::set_entry(const QString& e) { _entry=e; }

void FeatureBox::_seek()
{
  const QStringList& l = FeatureRegistry::instance().names();
  if (_entry.isEmpty() && l.size()>0)
    _entry = l[0];
  else
    for(int i=0; i<l.size(); i++)
      if (l[i]==_entry) {
	setCurrentIndex(i);
	break;
      }
}
