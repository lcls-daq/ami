#include "EnvPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>

#include <stdio.h>

using namespace Ami;


EnvPlot::EnvPlot(const DescEntry& output, 
		 unsigned input,
		 const char* feature) :
  AbsOperator(AbsOperator::EnvPlot),
  _input     (input),
  _cache     (0),
  _term      (0),
  _entry     (0)
{
  memcpy (_desc_buffer, &output, output.size());
  strncpy(_feature , feature   , FEATURE_LEN);
}

EnvPlot::EnvPlot(const char*& p, FeatureCache& features, const Cds& cds) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache (&features)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_input, sizeof(_input));
  _extract(p, _feature , FEATURE_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);

  QString expr(_feature);
  FeatureExpression parser;
  _term = parser.evaluate(features,expr);
  if (!_term)
    printf("EnvPlot failed to parse %s\n",qPrintable(expr));
}

EnvPlot::~EnvPlot()
{
  if (_term) delete _term;
  if (_entry) delete _entry;
}

unsigned   EnvPlot::input    () const { return _input; }

DescEntry& EnvPlot::output   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

const char* EnvPlot::feature() const { return _feature; }

void*      EnvPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_input, sizeof(_input));
  _insert(p, _feature , FEATURE_LEN);
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
      { bool damaged=false; double x=_term->evaluate();
	if (!damaged) {
	  EntryProf* en = static_cast<EntryProf*>(_entry);
	  en->addy(y,x);
	  en->addinfo(1.,EntryProf::Normalization);
	}
	break; }
    case DescEntry::Scan:    
      { bool damaged=false; double x=_term->evaluate();
	if (!damaged) {
	  EntryScan* en = static_cast<EntryScan*>(_entry);
	  en->addy(y,x);
	  en->addinfo(1.,EntryScan::Normalization);
	}
	break; }
    case DescEntry::Waveform:
    case DescEntry::TH2F:
    case DescEntry::Image:
    default:
      printf("EnvPlot::_operator no implementation for type %d\n",_entry->desc().type());
      break;
    }
    _entry->valid(e.time());
  }
  return *_entry;
}
