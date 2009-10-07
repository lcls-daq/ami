#include "EnvPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include <QtCore/QString>

#include <stdio.h>

using namespace Ami;


EnvPlot::EnvPlot(const DescEntry& output, 
		 unsigned input,
		 unsigned prof) :
  AbsOperator(AbsOperator::EnvPlot),
  _input     (input),
  _prof      (prof),
  _cache     (0),
  _entry     (0)
{
  memcpy (_desc_buffer, &output, output.size());
}

EnvPlot::EnvPlot(const char*& p, FeatureCache& features, const Cds& cds) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache (&features)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_input, sizeof(_input));
  _extract(p, &_prof , sizeof(_prof ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);
}

EnvPlot::~EnvPlot()
{
  if (_entry) delete _entry;
}

unsigned   EnvPlot::input    () const { return _input; }
unsigned   EnvPlot::prof     () const { return _prof ; }
DescEntry& EnvPlot::output   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      EnvPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_input, sizeof(_input));
  _insert(p, &_prof , sizeof(_prof ));
  return p;
}

Entry&     EnvPlot::_operate(const Entry& e) const
{
  bool dmg;
  double y = _cache->cache(_input,&dmg);
  if (!dmg) {
    switch(_entry->desc().type()) {
    case DescEntry::Scalar: 
      { EntryScalar* en = static_cast<EntryScalar*>(_entry);
	en->addcontent(y);    
	break; }
    case DescEntry::TH1F: 
      { EntryTH1F* en = static_cast<EntryTH1F*>(_entry);
	en->addcontent(1.,y); 
	en->addinfo(1.,EntryTH1F::Normalization);
	break; }
    case DescEntry::Prof:    
      { bool damaged; double x=_cache->cache(_prof,&damaged);
	if (!damaged) {
	  EntryProf* en = static_cast<EntryProf*>(_entry);
	  en->addy(y,x);
	  en->addinfo(1.,EntryProf::Normalization);
	}
	break; }
    case DescEntry::Waveform:
    case DescEntry::TH2F:
    case DescEntry::Image:
    default:
      printf("EnvPlot::_operator no implementation for type %d\n",_entry->desc().type());
      break;
    }
    _entry->time(e.time());
  }
  return *_entry;
}
