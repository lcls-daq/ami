#include "EnvPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntryW.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>

#include <stdio.h>

//#define DBUG

using namespace Ami;


EnvPlot::EnvPlot(const DescEntry& output) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache     (0),
  _term      (0),
  _weight    (0),
  _entry     (0),
  _input     (0),
  _v         (true)
{
  memcpy (_desc_buffer, &output, output.size());
  memset (_desc_buffer+output.size(), 0, DESC_LEN-output.size());
}

EnvPlot::EnvPlot(const char*& p, FeatureCache& features, const Cds& cds) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache     (&features),
  _term      (0),
  _weight    (0),
  _v         (true)
{
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);

#ifdef DBUG
  printf("EnvPlot ctor %s : %s\n",
	 o.name(),
	 o.xtitle());
#endif

  FeatureExpression parser;
  { QString expr(o.name());
    _input = parser.evaluate(features,expr);
    if (!_input) {
      printf("EnvPlot failed to parse input %s\n",qPrintable(expr));
      _v = false;
    }
  }

  if (o.type()==DescEntry::Prof ||
      o.type()==DescEntry::Scan ||
      o.type()==DescEntry::TH2F ||
      o.type()==DescEntry::ScalarDRange) {
    QString expr(o.xtitle());
    _term = parser.evaluate(features,expr);
    if (!_term) {
      printf("EnvPlot failed to parse term %s\n",qPrintable(expr));
      _v = false;
    }
  }

  if (o.isweighted_type()) {
    const DescEntryW& w = static_cast<const DescEntryW&>(o);
    if (w.weighted()) {
      QString expr(w.weight());
      printf("%s evaluates to\n",qPrintable(expr));
      _weight = parser.evaluate(features,expr);
      if (!_weight) {
	printf("EnvPlot failed to parse weight %s\n",qPrintable(expr));
        _v = false;
      }
    }
  }
}

EnvPlot::~EnvPlot()
{
  if (_input ) delete _input;
  if (_term  ) delete _term;
  if (_weight) delete _weight;
  if (_entry ) delete _entry;
}

DescEntry& EnvPlot::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      EnvPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

#include "pdsdata/xtc/ClockTime.hh"

Entry&     EnvPlot::_operate(const Entry& e) const
{
  if (_input != 0 && e.valid()) {
    Feature::damage(false);
    double y = _input->evaluate();
    double w = _weight ? _weight->evaluate() : 1;

#ifdef DBUG
    printf("EnvPlot::operate %s %s y %f  dmg %c\n", 
	   _entry->desc().name(), 
	   _entry->desc().xtitle(), 
	   y, Feature::damage() ? 't':'f');
#endif

    if (!Feature::damage()) {
      switch(_entry->desc().type()) {
      case DescEntry::Scalar: 
	{ EntryScalar* en = static_cast<EntryScalar*>(_entry);
	  en->addcontent(y);    
	  break; }
      case DescEntry::ScalarRange: 
	{ EntryScalarRange* en = static_cast<EntryScalarRange*>(_entry);
	  en->addcontent(y);    
	  break; }
      case DescEntry::ScalarDRange: 
	if (_term) {
	  Feature::damage(false);
	  double x=_term->evaluate();
	  if (!Feature::damage()) {
            EntryScalarDRange* en = static_cast<EntryScalarDRange*>(_entry);
            en->addcontent(x,y);    
          }
        }
        break;
      case DescEntry::TH1F: 
	{ EntryTH1F* en = static_cast<EntryTH1F*>(_entry);
	  en->addcontent(1.,y); 
	  en->addinfo(1.,EntryTH1F::Normalization);
	  break; }
      case DescEntry::Prof:    
	if (_term) {
	  Feature::damage(false);
	  double x=_term->evaluate();
	  if (!Feature::damage()) {
	    EntryProf* en = static_cast<EntryProf*>(_entry);
	    en->addy(y,x);
	    en->addinfo(1.,EntryProf::Normalization);
	  }
	}
	break;
      case DescEntry::Scan:    
	if (_term) {
	  Feature::damage(false);
	  double x=_term->evaluate();
	  if (!Feature::damage()) {
	    EntryScan* en = static_cast<EntryScan*>(_entry);
	    en->addy(y,x,w,e.last());
	    en->addinfo(1.,EntryScan::Normalization);
	  }
	} 
	break;
      case DescEntry::Cache:
        { EntryCache* en = static_cast<EntryCache*>(_entry);
          en->set(y,false); 
          break; }
      case DescEntry::TH2F:
	if (_term) {
	  Feature::damage(false);
	  double x=_term->evaluate();
	  if (!Feature::damage()) {
	    EntryTH2F* en = static_cast<EntryTH2F*>(_entry);
	    en->addcontent(1.,x,y);
	    en->addinfo(1.,EntryTH2F::Normalization);
	  }
	} 
        break;
      case DescEntry::Waveform:
      case DescEntry::Image:
      default:
	printf("EnvPlot::_operator no implementation for type %d\n",_entry->desc().type());
	break;
      }
      _entry->valid(e.time());
    }
  }
  return *_entry;
}
