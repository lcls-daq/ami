#include "ami/data/DescScalar.hh"

using namespace Ami;

DescScalar::DescScalar(const char* name, 
		       const char* ytitle) :
  DescEntry(name, "", ytitle, Scalar, sizeof(DescScalar), true)
{}

DescScalar::DescScalar(const Pds::DetInfo& info,
		       unsigned channel,
		       const char* name, 
		       const char* ytitle) :
  DescEntry(info, channel, name, "", ytitle, Scalar, sizeof(DescScalar), true)
{}
