#include "ami/data/DescRef.hh"

using namespace Ami;

DescRef::DescRef(const char* name) :
  DescEntry(name, "", "", Ref, sizeof(DescRef))
{
}

DescRef::DescRef(const Pds::DetInfo& info,
		 unsigned channel,
		 const char* name) :
  DescEntry(info, channel, name, "", "", Ref, sizeof(DescRef))
{
}
