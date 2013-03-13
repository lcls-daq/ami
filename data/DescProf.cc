#include "ami/data/DescProf.hh"

using namespace Ami;

DescProf::DescProf(const char* name, 
		   const char* xtitle, 
		   const char* ytitle, 
		   unsigned nbins, 
		   float xlow, 
		   float xup, 
		   const char* names,
		   const char* weight) :
  DescEntryW(name, xtitle, ytitle, weight, Prof, sizeof(DescProf), false),
  _nbins(nbins ? nbins : 1),
  _xlow(xlow),
  _xup(xup)
{
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    while (dst <= last)
      *dst++ = 0;
  } else {
    memset(_names, 0, NamesSize);
  }
}

DescProf::DescProf(InitOpt,
                   const char* name, 
		   const char* xtitle, 
		   const char* ytitle, 
		   unsigned nbins, 
		   float xlow, 
		   float xup, 
		   const char* names,
		   const char* weight) :
  DescEntryW(name, xtitle, ytitle, weight, Prof, sizeof(DescProf), false),
  _nbins(nbins ? nbins : 1)
{
  if (_nbins > 1) {
    double nn = 2*double(nbins)-1;
    double nd = 2*double(nbins-1);
    _xlow = (nn*xlow - xup )/nd;
    _xup  = (nn*xup  - xlow)/nd;
  }
  else {
    _xlow = xlow;
    _xup  = xup ;
  }
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    *dst++ = 0;
    *dst++ = 0;
  } else {
    _names[0] = 0;
  }
}

DescProf::DescProf(const Pds::DetInfo& info,
		   unsigned channel,
		   const char* name, 
		   const char* xtitle, 
		   const char* ytitle, 
		   unsigned nbins, 
		   float xlow, 
		   float xup, 
		   const char* names) :
  DescEntryW(info, channel, name, xtitle, ytitle, "", Prof, sizeof(DescProf), false),
  _nbins(nbins ? nbins : 1),
  _xlow(xlow),
  _xup(xup)
{
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    *dst++ = 0;
    *dst++ = 0;
  } else {
    _names[0] = 0;
  }
}

void DescProf::params(unsigned nbins, 
		      float xlow, 
		      float xup, 
		      const char* names)
{
  _nbins = nbins ? nbins : 1;
  _xlow = xlow;
  _xup = xup;
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    *dst++ = 0;
    *dst++ = 0;
  } else {
    _names[0] = 0;
  }
}
