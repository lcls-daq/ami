#include "ami/data/EntryRef.hh"

using namespace Ami;

EntryRef::~EntryRef() {}

EntryRef::EntryRef(const Pds::DetInfo& info, unsigned channel,
		   const char* name) :
  _desc(info, channel, name),
  _y   (allocate(sizeof(void*)))
{
}

EntryRef::EntryRef(const DescRef& desc) :
  _desc(desc),
  _y   (allocate(sizeof(void*)))
{
}
