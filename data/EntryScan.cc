#include "ami/data/EntryScan.hh"

#define SIZE(n) (sizeof(BinV)/sizeof(double)*n+InfoSize)

static void _dump(unsigned,const double*,const char*);

static const unsigned DefaultNbins = 100;

using namespace Ami;

EntryScan::~EntryScan() {}

EntryScan::EntryScan(const Pds::DetInfo& info, unsigned channel,
		     const char* name, 
		     const char* xtitle, 
		     const char* ytitle) :
  _desc(info, channel, name, xtitle, ytitle, DefaultNbins)
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

void EntryScan::addy(double y, double x, double w, double t) 
{
  unsigned bin = unsigned(info(Current));
  if (x != _p[bin]._x) {
    if (_p[bin]._nentries!=0)
      bin++;
    if (bin == _desc.nbins())
      bin = 0;

    _p[bin]._x        = x;
    _p[bin]._nentries = w;
    _p[bin]._ysum     = y*w;
    _p[bin]._y2sum    = y*y*w;
    _p[bin]._t        = t;
    info(bin,Current);
  }
  else
    addy(y,bin,w);
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

int EntryScan::_insert_bin(const BinV& bv, int& fb)
{
  int ib = desc().nbins()-1;
  while(ib >= fb) {
    if (_p[ib]._x == bv._x) {
      _p[ib]._nentries += bv._nentries;
      _p[ib]._ysum     += bv._ysum    ;
      _p[ib]._y2sum    += bv._y2sum   ;
      if (bv._t > _p[ib]._t)  _p[ib]._t = bv._t;
      return 1;
    }
    ib--;
  }
  if (ib < 0) return 0;

  _p[ib]._nentries = bv._nentries;
  _p[ib]._x        = bv._x       ;
  _p[ib]._ysum     = bv._ysum    ;
  _p[ib]._y2sum    = bv._y2sum   ;
  _p[ib]._t        = bv._t       ;
  fb = ib;
  return 1;
}

//
//  Merge two EntryScans' contents
//
void EntryScan::_sum(const BinV* a,
		     const BinV* b)
{
  unsigned nb = desc().nbins();
  const BinV* p_a = reinterpret_cast<const BinV*>(a);           // BinV array
  const BinV* p_b = reinterpret_cast<const BinV*>(b);
  const double* i_a = reinterpret_cast<const double*>(&p_a[nb]);  // Info array
  const double* i_b = reinterpret_cast<const double*>(&p_b[nb]);

  int fb = nb;
  int cb_a = int(i_a[Current]), last_a = (cb_a+1)%nb;    // Start, End in cicular array
  int cb_b = int(i_b[Current]), last_b = (cb_b+1)%nb;
  bool done_a = false, done_b = false;
  //
  //  Step through both entries arrays and add contents starting with the most recent (Current)
  //  Stop when the destination buffer is full
  //
  do {
    double t_a = done_a ? -1 : p_a[cb_a]._t;
    double t_b = done_b ? -1 : p_b[cb_b]._t;
    if (t_a > t_b) {
      if (!_insert_bin(p_a[cb_a],fb))
        break;
      cb_a = (cb_a==0) ? nb-1 : cb_a-1;
      done_a = (cb_a == last_a);
    }
    else {
      if (!_insert_bin(p_b[cb_b],fb))
        break;
      cb_b = (cb_b==0) ? nb-1 : cb_b-1;
      done_b = (cb_b == last_b);
    }
  } while(!done_a || !done_b);

  //  Clear bins that were never filled
  while( fb-- )
    _p[fb]._nentries = 0;

  info(double(nb-1), Current);
  info(i_a[Normalization]+i_b[Normalization],Normalization);

  //  valid( *a<*b ? *b : *a);
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

void EntryScan::add(const EntryScan& curr)
{
  EntryScan* t = new EntryScan(desc());
  t->_sum(_p,curr._p);
  memcpy(_p, t->_p, SIZE(_desc.nbins())*sizeof(double));
  delete t;

  if (curr.last() > last())
    valid(curr.time());
}	  

void EntryScan::_merge(char* p) const
{
  //  Consider scans that don't gather enough events to see all servers
  //  Consider bld that is different for every event
  BinV* dst = reinterpret_cast<BinV*>(p);
  const BinV* end = dst + _desc.nbins();
  const BinV* src = _p;
  
#if 0
  unsigned nb = _desc.nbins();
  _dump(nb,(double*)dst,"\n agg before");
  _dump(nb,(double*)src,"input");
#endif

  EntryScan* t = new EntryScan(_desc);
  t->_sum(dst,src);
  
  src = t->_p;

#if 0
  _dump(nb,(double*)src,"agg after");
#endif

  memcpy(dst, src, SIZE(_desc.nbins())*sizeof(double));

  delete t;
}	  

void EntryScan::dump() const
{
  printf("\nEntryScan %p %s time %g\n",this,_desc.name(),last());
  unsigned nb = desc().nbins();
  int bin = int(info(Current)), last = bin;
  do {
    bin = (bin+1)%nb;
    printf("bin %d\tx %f\tn %f\ty %f\tt %g\n",
	   bin, _p[bin]._x, _p[bin]._nentries, _p[bin]._ysum, _p[bin]._t);
  } while(bin != last);
}

void _dump(unsigned nb,const double* p,const char* title)
{
  class BinV { public: double _x; double _nentries; double _ysum; double _y2sum; double _t; };

  const BinV* _p = reinterpret_cast<const BinV*>(p);
  const double* _i = reinterpret_cast<const double*>(&_p[nb]);  // Info array

  int bin = int(_i[EntryScan::Current]), last = bin;

  const unsigned* t = reinterpret_cast<const unsigned*>(p-1);
  printf("%s time %d.%09d  current %d\n",title,t[1],t[0],bin);

  do {
    bin = (bin+1)%nb;
    if (_p[bin]._nentries)
      printf("bin %d\tx %f\tn %f\ty %f\tt %f\n",
	     bin, _p[bin]._x, _p[bin]._nentries, _p[bin]._ysum, _p[bin]._t);
  } while(bin != last);
}
