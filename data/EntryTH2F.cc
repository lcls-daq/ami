#include "ami/data/EntryTH2F.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;

using namespace Ami;

#define SIZE(nx,ny) (nx*ny+InfoSize)

EntryTH2F::~EntryTH2F() {}

EntryTH2F::EntryTH2F(const Pds::DetInfo& info, unsigned channel,
		     const char* name, 
		     const char* xtitle, 
		     const char* ytitle,
		     bool isnormalized) :
  _desc(info, channel, name, xtitle, ytitle, 
	DefaultNbins, DefaultLo, DefaultUp,
	DefaultNbins, DefaultLo, DefaultUp,
	isnormalized)
{
  build(DefaultNbins, DefaultNbins);
}

EntryTH2F::EntryTH2F(const DescTH2F& desc) :
  _desc(desc)
{
  build(_desc.nbinsx(), _desc.nbinsy());
}

void EntryTH2F::params(unsigned nbinsx, float xlow, float xup,
			  unsigned nbinsy, float ylow, float yup)
{
  _desc.params(nbinsx, xlow, xup, nbinsy, ylow, yup);
  build(nbinsx, nbinsy);
}

void EntryTH2F::params(const DescTH2F& desc)
{
  _desc = desc;
  build(_desc.nbinsx(), _desc.nbinsy());
}

void EntryTH2F::build(unsigned nbinsx, unsigned nbinsy)
{
  _y = static_cast<float*>(allocate(sizeof(float)*SIZE(nbinsx,nbinsy)));
}

const DescTH2F& EntryTH2F::desc() const {return _desc;}
DescTH2F& EntryTH2F::desc() {return _desc;}

void EntryTH2F::setto(const EntryTH2F& entry) 
{
  float* dst = _y;
  const float* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const float* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  valid(entry.time());
}

void EntryTH2F::setto(const EntryTH2F& curr, 
			 const EntryTH2F& prev)
{
  float* dst = _y;
  const float* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const float* srccurr = curr._y;
  const float* srcprev = prev._y;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  valid(curr.time());
}

