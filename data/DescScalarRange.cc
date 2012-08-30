#include "ami/data/DescScalarRange.hh"

using namespace Ami;

DescScalarRange::DescScalarRange(const char* name, 
                                 const char* ytitle,
                                 Stat        stat,
                                 double      extent,
                                 unsigned    nsamples,
                                 unsigned    nbins,
                                 bool        lnormalize) :
  DescEntry(name, "", ytitle, ScalarRange, sizeof(DescScalarRange), lnormalize),
  _extent  (extent),
  _nsamples(nsamples),
  _nbins   (nbins)
{
  options(unsigned(stat));
}

DescScalarRange::DescScalarRange(const DescScalarRange& o) :
  DescEntry(o),
  _extent  (o._extent),
  _nsamples(o._nsamples),
  _nbins   (o._nbins)
{
}
