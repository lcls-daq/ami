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
#ifdef DBUG
  printf("ESR:ctor  nbins %u\n",_desc.nbins());
#endif
}

double EntryScalarRange::entries() const { return _range->entries() - _desc.nsamples(); }

const DescScalarRange& EntryScalarRange::desc() const {return _desc;}
DescScalarRange& EntryScalarRange::desc() {return _desc;}

DescTH1F* EntryScalarRange::result(const void* cp) const
{
  void* p = const_cast<void*>(cp);

  double m,r;
  if (_desc.stat() == DescScalarRange::MinMax) {
    r = (0.5+_desc.extent())*(_range->max()-_range->min());
    if (r<=0) r = 0.5;
    m = 0.5*(_range->min()+_range->max());
  }
  else {
    r = _desc.extent()*_range->rms();
    if (r<=0) r = 0.5;
    m = _range->mean();
  }

  unsigned nbins = _desc.nbins();
  if (_desc.granularity()) {
    int dpb = int(2.*r/(_desc.granularity()*double(nbins))) + 1;
    //  Decrease the number of bins or increase the range
    r = 0.5*double(dpb*nbins)*_desc.granularity();
  }

  float xlow, xhigh;
  xlow  = m - r;
  xhigh = m + r;

  DescTH1F* d;
  if (p)
    d = new(p) DescTH1F(_desc.name(),"",_desc.ytitle(),
			nbins,xlow,xhigh,_desc.isnormalized());
  else
    d = new DescTH1F(_desc.name(),"",_desc.ytitle(),
		     nbins,xlow,xhigh,_desc.isnormalized());
  return d;
}
