#include "ami/data/DescScalar.hh"

using namespace Ami;

DescScalar::DescScalar(const char* name, 
		       const char* ytitle,
                       Stat        stat,
		       const char* weight,
                       unsigned    npts,
                       unsigned    pscal) :
  DescEntryW(name, "", ytitle, weight, Scalar, sizeof(DescScalar), stat, true),
  _npoints (npts),
  _prescale(pscal)
{
}

DescScalar::DescScalar(const Pds::DetInfo& info,
		       unsigned channel,
		       const char* name, 
		       const char* ytitle,
                       Stat        stat,
                       unsigned    npts,
                       unsigned    pscal) :
  DescEntryW(info, channel, name, "", ytitle, "", Scalar, sizeof(DescScalar), stat, true),
  _npoints (npts),
  _prescale(pscal)
{
}
