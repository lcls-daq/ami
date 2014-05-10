#include "ami/data/DescProf2D.hh"

using namespace Ami;

DescProf2D::DescProf2D(const char* name, 
                       const char* xtitle, 
                       const char* ytitle, 
                       unsigned nxbins, 
                       float xlow, 
                       float xup, 
                       unsigned nybins, 
                       float ylow, 
                       float yup, 
                       const char* names,
                       const char* weight) :
  DescEntryW(name, xtitle, ytitle, weight, Prof2D, sizeof(DescProf2D), false),
  _nxbins(nxbins ? nxbins : 1),
  _xlow(xlow),
  _xup(xup),
  _nybins(nybins ? nybins : 1),
  _ylow(ylow),
  _yup(yup)
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

DescProf2D::DescProf2D(InitOpt,
                       const char* name, 
                       const char* xtitle, 
                       const char* ytitle, 
                       unsigned nxbins, 
                       float xlow, 
                       float xup, 
                       unsigned nybins, 
                       float ylow, 
                       float yup, 
                       const char* names,
                       const char* weight) :
  DescEntryW(name, xtitle, ytitle, weight, Prof2D, sizeof(DescProf2D), false),
  _nxbins(nxbins ? nxbins : 1),
  _nybins(nybins ? nybins : 1)
{
  _setup_bins(nxbins,xlow,xup,_xlow,_xup);
  _setup_bins(nybins,ylow,yup,_ylow,_yup);
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

DescProf2D::DescProf2D(const Pds::DetInfo& info,
                       unsigned channel,
                       const char* name, 
                       const char* xtitle, 
                       const char* ytitle, 
                       unsigned nxbins, 
                       float xlow, 
                       float xup, 
                       unsigned nybins, 
                       float ylow, 
                       float yup, 
                       const char* names) :
  DescEntryW(info, channel, name, xtitle, ytitle, "", Prof2D, sizeof(DescProf2D), false),
  _nxbins(nxbins ? nxbins : 1),
  _xlow(xlow),
  _xup (xup),
  _nybins(nybins ? nybins : 1),
  _ylow(ylow),
  _yup (yup)
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

void DescProf2D::params(unsigned nxbins, 
                        float xlow, 
                        float xup, 
                        unsigned nybins,
                        float ylow, 
                        float yup, 
                        const char* names)
{
  _nxbins = nxbins ? nxbins : 1;
  _xlow = xlow;
  _xup  = xup;
  _nybins = nybins ? nybins : 1;
  _ylow = ylow;
  _yup  = yup;
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

void DescProf2D::_setup_bins(unsigned nxbins, 
                             float  xlo , float  xhi ,
                             float& xlo_, float& xhi_)
{
  if (nxbins > 1) {
    double nn = 2*double(nxbins)-1;
    double nd = 2*double(nxbins-1);
    xlo_ = (nn*xlo - xhi)/nd;
    xhi_ = (nn*xhi - xlo)/nd;
  }
  else {
    xlo_ = xlo;
    xhi_ = xhi;
  }
}
