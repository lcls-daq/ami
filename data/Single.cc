#include "Single.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include <stdio.h>

using namespace Ami;

Single::Single() : 
  AbsOperator(AbsOperator::Single),
  _entry     (0)
{
}

Single::Single(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::Single)
{
  _entry = EntryFactory::entry(e);
}

Single::~Single()
{
  if (_entry) delete _entry;
}

DescEntry& Single::output   () const { return _entry->desc(); }

void*      Single::_serialize(void* p) const
{
  return p;
}

#define SET_CASE(type) { \
  case DescEntry::type: \
    reinterpret_cast<Entry##type*>(_entry)->setto(reinterpret_cast<const Entry##type&>(e)); \
    break; }

Entry&     Single::_operate(const Entry& e) const
{
  switch(e.desc().type()) {
    SET_CASE(TH1F);
    SET_CASE(TH2F);
    SET_CASE(Prof);
    SET_CASE(Image);
    SET_CASE(Waveform);
  default:
    break;
  }
  return *_entry;
}

#undef SET_CASE
