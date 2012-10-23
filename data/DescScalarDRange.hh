#ifndef Pds_ENTRYDESCScalarDRange_HH
#define Pds_ENTRYDESCScalarDRange_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescScalarDRange : public DescEntry {
  public:
    enum Stat { MinMax, MeanSigma, Fixed };
    DescScalarDRange(const char* name, 
                     const char* xtitle,
                     const char* ytitle,
                     unsigned    nsamples,
                     Stat        statx,
                     double      extentx,
                     unsigned    nbinsx,
                     Stat        staty,
                     double      extenty,
                     unsigned    nbinsy,
                     bool        lnormalize=true);
    DescScalarDRange(const char* name, 
                     const char* xtitle,
                     const char* ytitle,
                     unsigned    nsamples,
                     Stat        statx,
                     double      extentx,
                     unsigned    nbinsx,
                     unsigned    nbinsy,
                     double      ylo,
                     double      yhi,
                     bool        lnormalize=true);
    DescScalarDRange(const char* name, 
                     const char* xtitle,
                     const char* ytitle,
                     unsigned    nsamples,
                     unsigned    nbinsx,
                     double      xlo,
                     double      xhi,
                     Stat        staty,
                     double      extenty,
                     unsigned    nbinsy,
                     bool        lnormalize=true);
    DescScalarDRange(const DescScalarDRange&);
  public:
    inline unsigned nsamples  () const;
    inline Stat     stat_x    () const;
    inline double   extent_x  () const;
    inline unsigned nbins_x   () const;
    inline Stat     stat_y    () const;
    inline double   extent_y  () const;
    inline unsigned nbins_y   () const;
    inline double   lo_x      () const;
    inline double   hi_x      () const;
    inline double   lo_y      () const;
    inline double   hi_y      () const;
  private:
    union {
      double   extent;
      float    bounds[2];
    } _x, _y;
    uint32_t _nsamples;
    uint16_t _nbins_x;
    uint16_t _nbins_y;
  };
};

unsigned Ami::DescScalarDRange::nsamples () const { return _nsamples; }

Ami::DescScalarDRange::Stat Ami::DescScalarDRange::stat_x() const
{
  return Ami::DescScalarDRange::Stat(options()&3); 
}

double   Ami::DescScalarDRange::extent_x() const { return _x.extent; }

unsigned Ami::DescScalarDRange::nbins_x () const { return _nbins_x; }

Ami::DescScalarDRange::Stat Ami::DescScalarDRange::stat_y() const
{
  return Ami::DescScalarDRange::Stat((options()>>2)&3); 
}

double   Ami::DescScalarDRange::extent_y() const { return _y.extent; }

unsigned Ami::DescScalarDRange::nbins_y () const { return _nbins_y; }

double   Ami::DescScalarDRange::lo_x      () const { return _x.bounds[0]; }
double   Ami::DescScalarDRange::hi_x      () const { return _x.bounds[1]; }
double   Ami::DescScalarDRange::lo_y      () const { return _y.bounds[0]; }
double   Ami::DescScalarDRange::hi_y      () const { return _y.bounds[1]; }

#endif
