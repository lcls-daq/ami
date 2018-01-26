#include "ami/data/EntryImage.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;
static const unsigned DefaultPedestal = 128;

using namespace Ami;

#define SIZE(nx,ny) (nx*ny+InfoSize*sizeof(float)/sizeof(uint32_t))

EntryImage::~EntryImage() {}

EntryImage::EntryImage(const Pds::DetInfo& info, unsigned channel, const char* name) :
  _desc(info, channel, name, DefaultNbins, DefaultNbins)
{
  build();
}

EntryImage::EntryImage(const DescImage& desc) :
  _desc(desc)
{
  build();
}

void EntryImage::params(unsigned nbinsx,
			   unsigned nbinsy,
			   int ppxbin,
			   int ppybin)
{
  _desc.params(nbinsx, nbinsy, ppxbin, ppybin);
  build();
}

void EntryImage::params(const DescImage& desc)
{
  _desc = desc;
  build();
}

void EntryImage::build()
{
  _y = static_cast<uint32_t*>(allocate(sizeof(uint32_t)*SIZE(_desc.nbinsx(), _desc.nbinsy())));
  info(DefaultPedestal, EntryImage::Pedestal);
}

const DescImage& EntryImage::desc() const {return _desc;}
DescImage& EntryImage::desc() {return _desc;}

void EntryImage::setto(const EntryImage& entry) 
{
  uint32_t* dst = _y;
  const uint32_t* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const uint32_t* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  valid(entry.time());
}

void EntryImage::setto(const EntryImage& curr, 
			  const EntryImage& prev)
{
  uint32_t* dst = _y;
  const uint32_t* end = dst+_desc.nbinsx()*_desc.nbinsy();
  const uint32_t* srccurr = curr._y;
  const uint32_t* srcprev = prev._y;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);

  float* fdst = reinterpret_cast<float*>(dst);
  const float* fcurr = reinterpret_cast<const float*>(srccurr);
  const float* fprev = reinterpret_cast<const float*>(srcprev);
  const float* fend = reinterpret_cast<const float*>(end)+InfoSize;
  do {
    *fdst++ = *fcurr++ - *fprev++;
  } while (fdst < fend);

  valid(curr.time());
}

void EntryImage::add  (const EntryImage& entry) 
{
  uint32_t* dst = _y;
  const uint32_t* end = dst+_desc.nbinsx()*_desc.nbinsy();
  const uint32_t* src = entry._y;
  do {
    *dst++ += *src++;
  } while (dst < end);

  float* fdst = reinterpret_cast<float*>(dst);
  const float* fsrc = reinterpret_cast<const float*>(src);
  const float* fend = reinterpret_cast<const float*>(end)+InfoSize;
  do {
    *fdst++ += *fsrc++;
  } while (fdst < fend);

  valid(entry.time());
}

void EntryImage::_merge  (char* p) const
{
  uint32_t* dst = reinterpret_cast<uint32_t*>(p);
  const uint32_t* end = dst+_desc.nbinsx()*_desc.nbinsy();
  const uint32_t* src = _y;
  do {
    *dst++ += *src++;
  } while (dst < end);

  float* fdst = reinterpret_cast<float*>(dst);
  const float* fsrc = reinterpret_cast<const float*>(src);
  const float* fend = reinterpret_cast<const float*>(end)+InfoSize;
  do {
    *fdst++ += *fsrc++;
  } while (fdst < fend);
}

ndarray<uint32_t,2> EntryImage::contents(unsigned fn)
{
  const SubFrame& f = _desc.frame(fn);
  unsigned shape[] = {f.ny,f.nx};
  ndarray<uint32_t,2> result(_y+f.x+f.y*_desc.nbinsx(),shape);

  int strides[] = {int(_desc.nbinsx()),1};
  result.strides(strides);

  return result;
}

const ndarray<const uint32_t,2> EntryImage::contents(unsigned fn) const
{
  const SubFrame& f = _desc.frame(fn);
  unsigned shape[] = {f.ny,f.nx};
  ndarray<const uint32_t,2> result(_y+f.x+f.y*_desc.nbinsx(),shape);

  int strides[] = {int(_desc.nbinsx()),1};
  result.strides(strides);

  return result;
}

ndarray<uint32_t,2> EntryImage::contents(const SubFrame& f)
{
  unsigned shape[] = {f.ny,f.nx};
  ndarray<uint32_t,2> result(_y+f.x+f.y*_desc.nbinsx(),shape);

  int strides[] = {int(_desc.nbinsx()),1};
  result.strides(strides);

  return result;
}

const ndarray<const uint32_t,2> EntryImage::contents(const SubFrame& f) const
{
  unsigned shape[] = {f.ny,f.nx};
  ndarray<const uint32_t,2> result(_y+f.x+f.y*_desc.nbinsx(),shape);

  int strides[] = {int(_desc.nbinsx()),1};
  result.strides(strides);

  return result;
}

template <class T>
static void _fill_image(const ndarray<const T,2>& a,
			EntryImage& entry)
{
  const DescImage& desc = entry.desc();
  const T* d = a.data();
  for(unsigned j=0; j<a.shape()[0]; j++) {
    unsigned iy = j/desc.ppybin();
    switch(desc.ppxbin()) {
    case 1:
      for(unsigned k=0; k<a.shape()[1]; k++)
	entry.addcontent(*d++, k, iy);
      break;
    case 2:
      for(unsigned k=0; k<a.shape()[1]; k++)
	entry.addcontent(*d++, k>>1, iy);
      break;
    case 4:
      for(unsigned k=0; k<a.shape()[1]; k++)
	entry.addcontent(*d++, k>>2, iy);
      break;
    default:
      for(unsigned k=0; k<a.shape()[1]; k++)
	entry.addcontent(*d++, k/desc.ppxbin(), iy);
      break;
    }
  }
}

void EntryImage::content(const ndarray<const uint32_t,2>& a)
{ _fill_image<uint32_t>(a,*this); }

void EntryImage::content(const ndarray<const int32_t,2>& a)
{ _fill_image<int32_t>(a,*this); }

void EntryImage::content(const ndarray<const uint16_t,2>& a)
{ _fill_image<uint16_t>(a,*this); }

void EntryImage::content(const ndarray<const int16_t,2>& a)
{ _fill_image<int16_t>(a,*this); }

void EntryImage::content(const ndarray<const uint8_t,2>& a)
{ _fill_image<uint8_t>(a,*this); }
