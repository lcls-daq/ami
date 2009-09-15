#include "ami/data/DescScalar.hh"

using namespace Ami;

DescScalar::DescScalar(const char* name, 
		       const char* ytitle) :
  DescEntry(name, "", ytitle, Scalar, sizeof(DescScalar), true)
{}
