#include "EnvPlot.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/valgnd.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>

#include <stdio.h>

//#define DBUG

using namespace Ami;


EnvPlot::EnvPlot(const DescEntry& output) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache     (0),
  _entry     (0),
  _input     (0),
  _v         (true)
{
  memcpy_val (_desc_buffer, &output, output.size(),DESC_LEN);
}

EnvPlot::EnvPlot(const char*&  p, 
		 FeatureCache& input, 
		 FeatureCache& output, 
		 const Cds&    cds) :
  AbsOperator(AbsOperator::EnvPlot),
  _cache     (&input),
  _input     (0),
  _v         (true)
{
  _extract(p, _desc_buffer, DESC_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o,&output);

#ifdef DBUG
  printf("EnvPlot ctor %s : %s\n",
	 o.name(),
	 o.xtitle());
#endif

  FeatureExpression parser;
  { QString expr(o.name());
    _input = parser.evaluate(input,expr);
    if (!_input) {
      printf("EnvPlot failed to parse input %s\n",qPrintable(expr));
      _v = false;
    }
  }

  _v &= _setup(o,input);
}

EnvPlot::~EnvPlot()
{
  if (_input ) delete _input;
  if (_entry ) delete _entry;
}

void EnvPlot::use() 
{
  _use();
  if (_input ) _input ->use();
}

const DescEntry& EnvPlot::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      EnvPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  return p;
}

#include "pdsdata/xtc/ClockTime.hh"

Entry&     EnvPlot::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_entry;

  if (_input != 0 && e.valid()) {
    double y = _input->evaluate();

#ifdef DBUG
    printf("EnvPlot::operate %s %s y %f  dmg %c\n", 
	   _entry->desc().name(), 
	   _entry->desc().xtitle(), 
	   y, Feature::damage() ? 't':'f');
#endif

    if (_input->valid()) {
      _fill(*_entry, y, e.time());
    }
    else {
#ifdef DBUG
      printf("EnvPlot [%s] input not valid\n",_entry->desc().name());
#endif
    }
  }
  return *_entry;
}

// represents data accumulate over several events
void EnvPlot::_invalid() {}
