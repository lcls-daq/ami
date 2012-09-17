#include "ami/data/EntryScalarRange.hh"
#include "ami/data/DescTH1F.hh"

#include <limits>
#include <new>

using namespace Ami;

EntryScalarRange::~EntryScalarRange() {}

EntryScalarRange::EntryScalarRange(const DescScalarRange& desc) :
  _desc(desc),
  _y   (static_cast<double*>(allocate(sizeof(double)*5)))
{
  _y[3] = std::numeric_limits<double>::max();
  _y[4] = std::numeric_limits<double>::min();
}

const DescScalarRange& EntryScalarRange::desc() const {return _desc;}
DescScalarRange& EntryScalarRange::desc() {return _desc;}

void EntryScalarRange::setto(const EntryScalarRange& entry) 
{
  _y[0] = entry._y[0];
  _y[1] = entry._y[1];
  _y[2] = entry._y[2];
  _y[3] = entry._y[3];
  _y[4] = entry._y[4];
}

void EntryScalarRange::add(const EntryScalarRange& entry) 
{
  _y[0] += entry._y[0];
  _y[1] += entry._y[1];
  _y[2] += entry._y[2];
  if (entry._y[3] < _y[3])
    _y[3] = entry._y[3];
  if (entry._y[4] > _y[4])
    _y[4] = entry._y[4];
}

DescTH1F* EntryScalarRange::result(void* p) const
{
  float xlow, xhigh;
  if (_desc.stat() == DescScalarRange::MinMax) {
    double r = _desc.extent()*(max()-min());
    if (r<=0) r = 0.5;
    xlow  = min() - r;
    xhigh = max() + r;
  }
  else {
    double r = _desc.extent()*rms();
    if (r<=0) r = 0.5;
    xlow  = mean() - r;
    xhigh = mean() + r;
  }
  if (p)
    return new(p) DescTH1F(_desc.name(),"",_desc.ytitle(),
                           _desc.nbins(),xlow,xhigh,_desc.isnormalized());
  else
    return new DescTH1F(_desc.name(),"",_desc.ytitle(),
                        _desc.nbins(),xlow,xhigh,_desc.isnormalized());
}
