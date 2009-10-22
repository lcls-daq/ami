#include "ami/data/EntryScan.hh"

#define SIZE(n) (4*n+InfoSize)

static const unsigned DefaultNbins = 100;

using namespace Ami;

EntryScan::~EntryScan() {}

EntryScan::EntryScan(const char* name, 
		     const char* xtitle, 
		     const char* ytitle) :
  _desc(name, xtitle, ytitle, DefaultNbins)
{
  build(DefaultNbins);
}

EntryScan::EntryScan(const DescScan& desc) :
  _desc(desc)
{
  build(desc.nbins());
}

void EntryScan::params(unsigned nbins)
{
  _desc.params(nbins);
  build(nbins);
}

void EntryScan::params(const DescScan& desc)
{
  _desc = desc;
  build(_desc.nbins());
}

void EntryScan::build(unsigned nbins)
{
  _p = reinterpret_cast<BinV*>(allocate(SIZE(nbins)*sizeof(double)));
}

const DescScan& EntryScan::desc() const {return _desc;}
DescScan& EntryScan::desc() {return _desc;}

void EntryScan::addy(double y, double x) 
{
  unsigned bin = unsigned(info(Current));
  if (x != _p[bin]._x) {
    bin++;
    if (bin == _desc.nbins())
      bin = 0;

    _p[bin]._x        = x;
    _p[bin]._nentries = 1;
    _p[bin]._ysum     = y;
    _p[bin]._y2sum    = y*y;
    info(bin,Current);
  }
  else
    addy(y,bin);
}

void EntryScan::setto(const EntryScan& entry) 
{
  double* dst = reinterpret_cast<double*>(_p);
  const double* end = dst + SIZE(_desc.nbins());
  const double* src = reinterpret_cast<const double*>(entry._p);
  do {
    *dst++ = *src++;
  } while (dst < end);
  valid(entry.time());
}

void EntryScan::diff(const EntryScan& curr, 
		     const EntryScan& prev) 
{
  double* dst = reinterpret_cast<double*>(_p);
  const double* end = dst + SIZE(_desc.nbins());
  const double* srccurr = reinterpret_cast<const double*>(curr._p);
  const double* srcprev = reinterpret_cast<const double*>(prev._p);
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  valid(curr.time());
}

void EntryScan::sum(const EntryScan& curr, 
		    const EntryScan& prev) 
{
  double* dst = reinterpret_cast<double*>(_p);
  const double* end = dst + SIZE(_desc.nbins());
  const double* srccurr = reinterpret_cast<const double*>(curr._p);
  const double* srcprev = reinterpret_cast<const double*>(prev._p);
  do {
    *dst++ = *srccurr++ + *srcprev++;
  } while (dst < end);
  valid(curr.time());
}
