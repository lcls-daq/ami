#include "ami/data/ScalarPlot.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntryW.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/FeatureExpression.hh"
#include "pdsdata/xtc/ClockTime.hh"

//#define DBUG

using namespace Ami;

ScalarPlot::ScalarPlot() :
  _xterm (0),
  _yterm (0),
  _weight(0),
  _x(0), _y(0), _w(1),
  _xterm_uses (false),
  _yterm_uses (false),
  _weight_uses(false)
{
}

ScalarPlot::~ScalarPlot()
{
  if (_xterm ) delete _xterm;  
  if (_yterm ) delete _yterm;
  if (_weight) delete _weight;
}

void ScalarPlot::_use()
{
  if (_xterm ) _xterm ->use();
  if (_yterm ) _yterm ->use();
  if (_weight) _weight->use();
}

Term* ScalarPlot::_process_expr(FeatureCache& input,
                                const char* expr,
                                bool& expr_uses)
{
  expr_uses=false;
  FeatureExpression parser;
  Term* t = parser.evaluate(input,expr);
  if (!t)
    printf("ScalarPlot failed to parse %s\n",expr);
  return t;
}

bool ScalarPlot::_setup(const DescEntry& o,
                        FeatureCache& input)
{
  if (o.type()==DescEntry::Prof ||
      o.type()==DescEntry::Prof2D ||
      o.type()==DescEntry::Scan ||
      o.type()==DescEntry::TH2F ||
      o.type()==DescEntry::ScalarDRange) {
    _xterm = _process_expr(input,o.xtitle(),_xterm_uses);
    if (!_xterm) return false;
  }

  if (o.type()==DescEntry::Prof2D) {
    _yterm = _process_expr(input,o.ytitle(),_yterm_uses);
    if (!_yterm) return false;
  }

  if (o.isweighted_type()) {
    const DescEntryW& w = static_cast<const DescEntryW&>(o);
    if (w.weighted()) {
      _weight = _process_expr(input,w.weight(),_weight_uses);
      if (!_weight) return false;
    }
#ifdef DBUG    
    printf("ScalarPlot[%p] weighted_type: weighted %c [%s] [%p]\n",
           this, w.weighted()?'T':'F', o.name(), _weight);
#endif
  }

  return true;
}

void ScalarPlot::_fill(Entry& entry, 
                       double v, 
                       const Pds::ClockTime& t,
                       bool initial) const
{
#ifdef DBUG
  printf("ScalarPlot[%p][%s] _fill %f _xterm %p\n",
	 this, entry.desc().name(), v, _xterm);
#endif

  if ( (initial || _xterm_uses) && _xterm ) {
    _x = _xterm->evaluate();
    if (!_xterm->valid()) return;
  }
  if ( (initial || _yterm_uses) && _yterm ) {
    _y = _yterm->evaluate();
    if (!_yterm->valid()) return;
  }
  if ( (initial || _weight_uses) && _weight ) {
    if ((_w = _weight->evaluate())<=0) return;
    if (!_weight->valid()) return;
  }

#ifdef DBUG
  if (entry.desc().isweighted_type()) {
    printf("ScalarPlot[%p] _fill %f, _w %f, _weight %p, uses %c\n",
           this, v, _w, _weight, _weight_uses?'T':'F');
  }
#endif

  switch(entry.desc().type()) {
  case DescEntry::Scalar: 
    { EntryScalar& en = static_cast<EntryScalar&>(entry);
      en.addcontent(v,_w);    
      break; }
  case DescEntry::ScalarRange: 
    { EntryScalarRange& en = static_cast<EntryScalarRange&>(entry);
      en.addcontent(v);    
      break; }
  case DescEntry::ScalarDRange: 
    { EntryScalarDRange& en = static_cast<EntryScalarDRange&>(entry);
      en.addcontent(_x,v);    
      break; }
  case DescEntry::TH1F: 
    { EntryTH1F& en = static_cast<EntryTH1F&>(entry);
      en.addcontent(1.,v); 
      en.addinfo(1.,EntryTH1F::Normalization);
      break; }
  case DescEntry::Prof:    
    { EntryProf& en = static_cast<EntryProf&>(entry);
      en.addy(v,_x,_w);
      en.addinfo(1.,EntryProf::Normalization);
    }
    break;
  case DescEntry::Prof2D:    
    { EntryProf2D& en = static_cast<EntryProf2D&>(entry);
      en.addz(v,_x,_y,_w);
      en.addinfo(1.,EntryProf2D::Normalization);
    }
    break;
  case DescEntry::Scan:    
    { EntryScan& en = static_cast<EntryScan&>(entry);
      en.addy(v,_x,_w,t.asDouble());
      en.addinfo(1.,EntryScan::Normalization);
    } 
    break;
  case DescEntry::Cache:
    { EntryCache& en = static_cast<EntryCache&>(entry);
      en.set(v,false); 
      break; }
  case DescEntry::TH2F:
    { EntryTH2F& en = static_cast<EntryTH2F&>(entry);
      en.addcontent(1.,_x,v);
      en.addinfo(1.,EntryTH2F::Normalization);
    } 
    break;
  case DescEntry::Waveform:
  case DescEntry::Image:
  default:
    printf("ScalarPlot::_operator no implementation for type %d\n",entry.desc().type());
    break;
  }
  entry.valid(t);
}
