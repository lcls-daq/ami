#include "ami/data/EntryProf2D.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;
static const char* DefaultNames = 0;

using namespace Ami;

#define SIZE(nbx,nby) (3*nbx*nby+InfoSize)

EntryProf2D::~EntryProf2D() {}

EntryProf2D::EntryProf2D(const Pds::DetInfo& info, unsigned channel, 
                         const char* name, 
                         const char* xtitle, 
                         const char* ytitle) :
  _desc(info, channel, name, xtitle, ytitle, 
        DefaultNbins, DefaultLo, DefaultUp, 
        DefaultNbins, DefaultLo, DefaultUp, 
        DefaultNames)
{
  build(DefaultNbins,DefaultNbins);
}

EntryProf2D::EntryProf2D(const DescProf2D& desc) :
  _desc(desc)
{
  build(desc.nxbins(),desc.nybins());
}

void EntryProf2D::params(unsigned nxbins, 
                         float xlow, 
                         float xup, 
                         unsigned nybins, 
                         float ylow, 
                         float yup, 
                         const char* names)
{
  _desc.params(nxbins, xlow, xup, nybins, ylow, yup, names);
  build(nxbins,nybins);
}

void EntryProf2D::params(const DescProf2D& desc)
{
  _desc = desc;
  build(_desc.nxbins(),_desc.nybins());
}

void EntryProf2D::build(unsigned nxbins,unsigned nybins)
{
  double* p = static_cast<double*>(allocate(SIZE(nxbins,nybins)*sizeof(double)));
  _zsum     = make_ndarray<double>(p,nybins,nxbins);
  _z2sum    = make_ndarray<double>(_zsum.end(),nybins,nxbins);
  _nentries = make_ndarray<double>(_z2sum.end(),nybins,nxbins);
}

const DescProf2D& EntryProf2D::desc() const {return _desc;}
DescProf2D& EntryProf2D::desc() {return _desc;}

void EntryProf2D::setto(const EntryProf2D& entry) 
{
  double* dst = _zsum.begin();
  const double* end = dst + SIZE(_desc.nxbins(),_desc.nybins());
  const double* src = entry._zsum.begin();
  do {
    *dst++ = *src++;
  } while (dst < end);
  valid(entry.time());
}

void EntryProf2D::diff(const EntryProf2D& curr, 
                       const EntryProf2D& prev) 
{
  double* dst = _zsum.begin();
  const double* end = dst + SIZE(_desc.nxbins(),_desc.nybins());
  const double* srccurr = curr._zsum.begin();
  const double* srcprev = prev._zsum.end();
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  valid(curr.time());
}

void EntryProf2D::sum(const EntryProf2D& curr, 
                      const EntryProf2D& prev) 
{
  double* dst = _zsum.begin();
  const double* end = dst + SIZE(_desc.nxbins(),_desc.nybins());
  const double* srccurr = curr._zsum.begin();
  const double* srcprev = prev._zsum.begin();
  do {
    *dst++ = *srccurr++ + *srcprev++;
  } while (dst < end);
  valid(curr.time());
}

void EntryProf2D::add(const EntryProf2D& curr)
{
  double* dst = _zsum.begin();
  const double* end = dst + SIZE(_desc.nxbins(),_desc.nybins());
  const double* src = curr._zsum.begin();
  do {
    *dst++ += *src++;
  } while (dst < end);
  valid(curr.time());
}

void EntryProf2D::_merge(char* p) const
{
  double* dst = reinterpret_cast<double*>(p);
  const double* end = dst + SIZE(_desc.nxbins(),_desc.nybins());
  const double* src = _zsum.begin();
  do {
    *dst++ += *src++;
  } while (dst < end);
}

void EntryProf2D::addz(double z, double x, double y, double w)
{
  bool lpass=false;
  int xbin = int((x-_desc.xlow())*double(_desc.nxbins())/(_desc.xup()-_desc.xlow()));
  if (xbin < 0)      
    addinfo(z,XUnderflow);
  else if ((unsigned)xbin >= _desc.nxbins())
    addinfo(z,XOverflow);
  else 
    lpass=true;

  int ybin = int((y-_desc.ylow())*double(_desc.nybins())/(_desc.yup()-_desc.ylow()));
  if (ybin < 0)      
    addinfo(z,YUnderflow);
  else if ((unsigned)ybin >= _desc.nybins())
    addinfo(z,YOverflow);
  else if (lpass)
    addz   (z,(unsigned)xbin,(unsigned)ybin,w);
}
