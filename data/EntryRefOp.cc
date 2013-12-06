#include "EntryRefOp.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryRef.hh"

#include <sys/uio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace Ami;

EntryRefOp::EntryRefOp(unsigned index) : 
  AbsOperator(AbsOperator::EntryRefOp),
  _index     (index),
  _output    (0)
{
}

EntryRefOp::EntryRefOp(const char*& p, const Entry& e) :
  AbsOperator(AbsOperator::EntryRefOp)
{
  _extract(p, &_index, sizeof(_index));
  
  const EntryRef& ref = static_cast<const EntryRef&>(e);
  Entry** entries = reinterpret_cast<Entry**>(const_cast<void*>(ref.data()));
  _output = &entries[_index]->desc();
}

EntryRefOp::~EntryRefOp()
{
}

static DescRef _no_entry("NoEntry");

DescEntry& EntryRefOp::_routput   () const 
{ 
  return *_output;
}

void*      EntryRefOp::_serialize(void* p) const
{
  _insert(p, &_index, sizeof(_index));
  return p;
}

Entry&     EntryRefOp::_operate(const Entry& e) const
{
  const EntryRef& ref = static_cast<const EntryRef&>(e);
  //  const cast is OK because this operator's output is always
  //  just input to another operator.
  Entry** p = reinterpret_cast<Entry**>(const_cast<void*>(ref.data()));
  return *(p[_index]);
}

void EntryRefOp::_invalid() {}
