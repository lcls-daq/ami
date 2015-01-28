#include "ami/data/LineFit.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"

#include "ami/data/Cds.hh"
#include "ami/data/valgnd.hh"
#include "ami/data/FeatureExpression.hh"

#include "ami/data/LeastSquares.hh"
#include "ami/data/MedianSlope.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <sstream>

#include <stdio.h>

//#define DBUG

using namespace Ami;

static const char _delim = '%';

LineFit::LineFit(const DescEntry& output, Method method) :
  AbsOperator(AbsOperator::LineFit),
  _method    (method),
  _cache     (0),
  _entry     (0),
  _xline     (0),
  _yline     (0),
  _v         (true),
  _acc       (0)
{
  memcpy_val (_desc_buffer, &output, output.size(),DESC_LEN);
}

LineFit::LineFit(const char*&  p) :
  AbsOperator(AbsOperator::LineFit),
  _cache     (0),
  _xline     (0),
  _yline     (0),
  _v         (true)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_method, sizeof(_method));
}

LineFit::LineFit(const char*&  p, 
		 FeatureCache& input) :
  AbsOperator(AbsOperator::LineFit),
  _cache     (&input),
  _xline     (0),
  _yline     (0),
  _v         (true)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_method, sizeof(_method));

  DescEntryW& o = *reinterpret_cast<DescEntryW*>(_desc_buffer);
  *const_cast<char*>(o.weight())=0;  // remove weight

  _entry = EntryFactory::entry(o);

  switch(_method) {
  case LeastSquares:
    _acc = new Ami::LeastSquares(_entry->desc());
    break;
  case MedianSlope:
    _acc = new Ami::MedianSlope(_entry->desc());
    break;
  default:
    break;
  };

#ifdef DBUG
  printf("LineFit ctor %s : %s\n",
	 o.name(),
	 o.xtitle());
#endif

  FeatureExpression parser;
  { QString expr(o.name());

    QStringList terms = expr.split(_delim);

    _xline = parser.evaluate(input,terms[0]);
    if (!_xline) {
      printf("LineFit failed to parse x %s\n",qPrintable(terms[0]));
      _v = false;
    }

    _yline = parser.evaluate(input,terms[1]);
    if (!_yline) {
      printf("LineFit failed to parse y %s\n",qPrintable(terms[1]));
      _v = false;
    }
  }

  _v &= _setup(o,input);
}

LineFit::~LineFit()
{
  if (_xline ) delete _xline;
  if (_yline ) delete _yline;
  if (_entry ) delete _entry;
  if (_acc   ) delete _acc;
}

void LineFit::use() 
{
  if (_xline ) _xline ->use();
  if (_yline ) _yline ->use();
  _use();
}

const DescEntry& LineFit::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      LineFit::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_method, sizeof(_method));
  return p;
}

#include "pdsdata/xtc/ClockTime.hh"

Entry&     LineFit::_operate(const Entry& e) const
{
  Entry& entry = *_entry;

  while(1) {  // Not a loop, just a convenient break/jump
    if (!e.valid())
      break;

    if (_xline != 0 && _yline!=0) {
      double x = _xline->evaluate();
      double y = _yline->evaluate();

      if (_xline->valid() && _yline->valid()) {

	if ( _xterm ) {
	  _x = _xterm->evaluate();
	  if (!_xterm->valid()) { break; }
	}
	if ( _yterm ) {
	  _y = _yterm->evaluate();
	  if (!_yterm->valid()) { break; }
	}
	if ( _weight ) {
	  _w = _weight->evaluate();
	  if (!_weight->valid()) break;
	}

	//  Accumulation of fit inputs goes below here
	const Pds::ClockTime& t = e.time();

	switch(entry.desc().type()) {
	case DescEntry::Scalar: 
	  { EntryScalar& en = static_cast<EntryScalar&>(entry);
	    _acc->add(en,x,y);
	    break; }
	case DescEntry::Prof:    
	  { EntryProf& en = static_cast<EntryProf&>(entry);
	    _acc->add(en,x,y,_x);
	    en.addinfo(1.,EntryProf::Normalization);
	  }
	  break;
	case DescEntry::Prof2D:    
	  { EntryProf2D& en = static_cast<EntryProf2D&>(entry);
	    _acc->add(en,x,y,_x,_y);
	    en.addinfo(1.,EntryProf2D::Normalization);
	  }
	  break;
	case DescEntry::Scan:    
	  { EntryScan& en = static_cast<EntryScan&>(entry);
	    _acc->add(en,x,y,_x,t.asDouble());
	    en.addinfo(1.,EntryScan::Normalization);
	  } 
	  break;
	default:
	  printf("ScalarPlot::_operator no implementation for type %d\n",entry.desc().type());
	  break;
	}
	entry.valid(t);
      }
    }
    else {
    }
    break;
  }

  return entry;
}

// represents data accumulate over several events
void LineFit::_invalid() {}

std::string LineFit::title(const char* x, const char* y, const char* stat)
{
  std::stringstream s;
  s << x << _delim << y << _delim << stat;
  return s.str();
}

static const char* _methods[] = { "LeastSquares", "MedianSlope", NULL };

const char* LineFit::method_str(Method m) 
{
  return _methods[m];
}

LineFit::Method LineFit::method(const char* s)
{
  for(unsigned i=0; i<NumberOf; i++)
    if (strcmp(s,_methods[i])==0)
      return Method(i);
  return NumberOf;
}

