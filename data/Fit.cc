#include "ami/data/Fit.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/FitEntry.hh"

#include <string.h>

using namespace Ami;

static const char* _functions[] = { "Gauss", "Lorentz" };

const char* Fit::function_str(Function m) 
{
  return _functions[m];
}

Fit::Function Fit::function(const char* s)
{
  for(unsigned i=0; i<NumberOf; i++)
    if (strcmp(s,_functions[i])==0)
      return Function(i);
  return NumberOf;
}

Fit::Fit(const DescEntry& output, 
         Function         function,
         unsigned         parameter) :
  AbsOperator(AbsOperator::Fit),
  _function  (function),
  _parameter (parameter),
  _cache     (0),
  _entry     (0),
  _fit       (0)
{
  memcpy_val (_desc_buffer, &output, output.size(), DESC_LEN);
}

Fit::Fit(const char*&  p,
         FeatureCache& input, 
         FeatureCache& output) :
  AbsOperator(AbsOperator::Fit),
  _cache     (&input),
  _v         (true),
  _fit       (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_function  , sizeof(_function ));
  _extract(p, &_parameter , sizeof(_parameter));

  _fit = FitEntry::instance(Function(_function));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o,&output);

  _v = _setup(o, input);
}

Fit::Fit(const char*& p) :
  AbsOperator(AbsOperator::Fit),
  _cache(0),
  _entry(0),
  _v    (true),
  _fit  (0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_function  , sizeof(_function ));
  _extract(p, &_parameter , sizeof(_parameter));
}

Fit::~Fit()
{
  if (_entry) delete _entry;
  if (_fit  ) delete _fit;
}

void Fit::use() { _use(); }

Fit::Function  Fit::function      () const { return _function; }

unsigned       Fit::parameter     () const { return _parameter; }

const DescEntry& Fit::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
}

void*      Fit::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_function  , sizeof(_function));
  _insert(p, &_parameter , sizeof(_parameter));
  return p;
}

void       Fit::_invalid() { _entry->invalid(); }

Entry&     Fit::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_entry;

  switch(e.desc().type()) {
  case DescEntry::TH1F: 
    { const EntryTH1F& entry = static_cast<const EntryTH1F&>(e);
      _fit->fit(entry);
    } break;
  case DescEntry::Prof: 
    { const EntryProf& entry = static_cast<const EntryProf&>(e);
      _fit->fit(entry);
    } break;
  default: 
    printf("Fit on type %s not implemented\n",
           DescEntry::type_str(e.desc().type()));
    return *_entry;
  }

  double y = _fit->params()[_parameter];

  _fill(*_entry, y, e.time());

  return *_entry;
}


