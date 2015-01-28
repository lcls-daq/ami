#ifndef Pds_ENTRYDESCScalar_HH
#define Pds_ENTRYDESCScalar_HH

#include "ami/data/DescEntryW.hh"

namespace Ami {

  class DescScalar : public DescEntryW {
  public:
    DescScalar(const char* name, 
	       const char* ytitle,
               Stat        stat=Mean,
	       const char* weight="",
               unsigned    npts=400,
               unsigned    pscal=1);

    DescScalar(const Pds::DetInfo& info,
	       unsigned channel,
	       const char* name, 
	       const char* ytitle,
               Stat        stat=Mean,
               unsigned    npts=400,
               unsigned    pscal=1);
  public:
    inline unsigned npoints () const;
    inline unsigned prescale() const;
  private:
    uint32_t _npoints;
    uint32_t _prescale;
  };
};

unsigned Ami::DescScalar::npoints() const { return _npoints; }

unsigned Ami::DescScalar::prescale() const { return _prescale; }

#endif
