#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/DescTH2F.hh"

#include <limits>
#include <new>

using namespace Ami;

EntryScalarDRange::~EntryScalarDRange() {}

EntryScalarDRange::EntryScalarDRange(const DescScalarDRange& desc) :
  _desc (desc)
{
  void* p = allocate(2*sizeof(ScalarRange));
  _x = new(p) ScalarRange;
  _y = new((void*)(_x+1)) ScalarRange;
}

const DescScalarDRange& EntryScalarDRange::desc() const {return _desc;}
DescScalarDRange& EntryScalarDRange::desc() {return _desc;}

double EntryScalarDRange::entries() const { return _x->entries() - _desc.nsamples(); }

#include <stdio.h>

DescTH2F* EntryScalarDRange::result(void* p) const
{
#define GET_RANGE(xlow,xhigh,range,p) {                                 \
    switch(_desc.stat_##p()) {                                          \
    case DescScalarDRange::MinMax : {                                   \
      double r = (0.5+_desc.extent_##p())*(range->max()-range->min());  \
      if (r<=0) r = 0.5;                                                \
      double m = 0.5*(range->min()+range->max());                       \
      xlow  = m - r;                                                    \
      xhigh = m + r;                                                    \
      break; }                                                          \
    case DescScalarDRange::MeanSigma:  {                                \
      double r = _desc.extent_##p()*range->rms();                       \
      if (r<=0) r = 0.5;                                                \
      xlow  = range->mean() - r;                                        \
      xhigh = range->mean() + r;                                        \
      break; }                                                          \
    default:                                                            \
      xlow  = _desc.lo_##p();                                           \
      xhigh = _desc.hi_##p();                                           \
      break;                                                            \
    }                                                                   \
  }

  float xlo, xhi;
  float ylo, yhi;

  GET_RANGE(xlo,xhi,_x,x);
  GET_RANGE(ylo,yhi,_y,y);

#undef GET_RANGE

  if (p)
    return new(p) DescTH2F(_desc.name(),_desc.xtitle(),_desc.ytitle(),
                           _desc.nbins_x(),xlo,xhi,
                           _desc.nbins_y(),ylo,yhi,
                           _desc.isnormalized());
  else
    return new    DescTH2F(_desc.name(),_desc.xtitle(),_desc.ytitle(),
                           _desc.nbins_x(),xlo,xhi,
                           _desc.nbins_y(),ylo,yhi,
                           _desc.isnormalized());
}
