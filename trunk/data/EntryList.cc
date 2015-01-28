#include "ami/data/EntryList.hh"

using namespace Ami;

EntryList::EntryList(Option o)
{
  if (o==Empty)
    _m = 0;
  else
    _m = -1;
}

EntryList::EntryList(uint32_t lower, uint32_t upper) 
{
  _m = upper;
  _m <<= 32;
  _m += lower;
}
