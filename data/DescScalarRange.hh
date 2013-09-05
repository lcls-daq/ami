#ifndef Pds_ENTRYDESCScalarRange_HH
#define Pds_ENTRYDESCScalarRange_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescScalarRange : public DescEntry {
  public:
    enum Stat { MinMax, MeanSigma };
    DescScalarRange(const char* name, 
                    const char* ytitle,
                    Stat        stat,
                    double      extent,
                    unsigned    nsamples,
                    unsigned    nbins,
                    bool        lnormalize=true,
		    double      granularity=0.);
    DescScalarRange(const DescScalarRange&);
  public:
    inline Stat     stat    () const;
    inline double   extent  () const;
    inline unsigned nsamples() const;
    inline unsigned nbins   () const;
    inline double   granularity() const;
  private:
    double   _extent;
    double   _granularity;
    uint16_t _nsamples;
    uint16_t _nbins;
  };
};

Ami::DescScalarRange::Stat Ami::DescScalarRange::stat() const
{
  return Ami::DescScalarRange::Stat(options()); 
}

double   Ami::DescScalarRange::extent() const { return _extent; }

double   Ami::DescScalarRange::granularity() const { return _granularity; }

unsigned Ami::DescScalarRange::nsamples () const { return _nsamples; }

unsigned Ami::DescScalarRange::nbins () const { return _nbins; }

#endif
