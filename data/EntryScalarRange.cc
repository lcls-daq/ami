#include "ami/data/EntryScalarRange.hh"
#include "ami/data/DescTH1F.hh"

#include <limits>
#include <new>

using namespace Ami;

EntryScalarRange::~EntryScalarRange() {}

EntryScalarRange::EntryScalarRange(const DescScalarRange& desc) :
  _desc (desc)
{
  _range = new(allocate(sizeof(ScalarRange))) ScalarRange;
}

double EntryScalarRange::entries() const { return _range->entries() - _desc.nsamples(); }

const DescScalarRange& EntryScalarRange::desc() const {return _desc;}
DescScalarRange& EntryScalarRange::desc() {return _desc;}

DescTH1F* EntryScalarRange::result(void* p) const
{
  float xlow, xhigh;
  if (_desc.stat() == DescScalarRange::MinMax) {
    double r = (0.5+_desc.extent())*(_range->max()-_range->min());
    if (r<=0) r = 0.5;
    double m = 0.5*(_range->min()+_range->max());
    xlow  = m - r;
    xhigh = m + r;
  }
  else {
    double r = _desc.extent()*_range->rms();
    if (r<=0) r = 0.5;
    xlow  = _range->mean() - r;
    xhigh = _range->mean() + r;
  }
  if (p)
    return new(p) DescTH1F(_desc.name(),"",_desc.ytitle(),
                           _desc.nbins(),xlow,xhigh,_desc.isnormalized());
  else
    return new DescTH1F(_desc.name(),"",_desc.ytitle(),
                        _desc.nbins(),xlow,xhigh,_desc.isnormalized());
}
