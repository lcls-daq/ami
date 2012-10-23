#include "ami/data/DescScalarDRange.hh"

using namespace Ami;

DescScalarDRange::DescScalarDRange(const char* name, 
                                   const char* xtitle,
                                   const char* ytitle,
                                   unsigned    nsamples,
                                   Stat        stat_x,
                                   double      extent_x,
                                   unsigned    nbins_x,
                                   Stat        stat_y,
                                   double      extent_y,
                                   unsigned    nbins_y,
                                   bool        lnormalize) :
  DescEntry(name, xtitle, ytitle, ScalarDRange, sizeof(DescScalarDRange), lnormalize),
  _nsamples  (nsamples),
  _nbins_x   (nbins_x),
  _nbins_y   (nbins_y)
{
  options(unsigned(stat_x)|(unsigned(stat_y)<<2));
  _x.extent = extent_x;
  _y.extent = extent_y;
}

DescScalarDRange::DescScalarDRange(const char* name, 
                                   const char* xtitle,
                                   const char* ytitle,
                                   unsigned    nsamples,
                                   Stat        stat_x,
                                   double      extent_x,
                                   unsigned    nbins_x,
                                   unsigned    nbins_y,
                                   double      ylo,
                                   double      yhi,
                                   bool        lnormalize) :
  DescEntry(name, xtitle, ytitle, ScalarDRange, sizeof(DescScalarDRange), lnormalize),
  _nsamples  (nsamples),
  _nbins_x   (nbins_x),
  _nbins_y   (nbins_y)
{
  options(unsigned(stat_x)|(unsigned(Fixed)<<2));
  _x.extent = extent_x;
  _y.bounds[0] = ylo;
  _y.bounds[1] = yhi;
}

DescScalarDRange::DescScalarDRange(const char* name, 
                                   const char* xtitle,
                                   const char* ytitle,
                                   unsigned    nsamples,
                                   unsigned    nbins_x,
                                   double      xlo,
                                   double      xhi,
                                   Stat        stat_y,
                                   double      extent_y,
                                   unsigned    nbins_y,
                                   bool        lnormalize) :
  DescEntry(name, xtitle, ytitle, ScalarDRange, sizeof(DescScalarDRange), lnormalize),
  _nsamples  (nsamples),
  _nbins_x   (nbins_x),
  _nbins_y   (nbins_y)
{
  options(unsigned(Fixed)|(unsigned(stat_y)<<2));
  _x.bounds[0] = xlo;
  _x.bounds[1] = xhi;
  _y.extent = extent_y;
}

DescScalarDRange::DescScalarDRange(const DescScalarDRange& o) :
  DescEntry(o),
  _nsamples  (o._nsamples),
  _nbins_x   (o._nbins_x),
  _nbins_y   (o._nbins_y)
{
  _x.extent = o._x.extent;
  _y.extent = o._y.extent;
}
