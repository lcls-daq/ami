#ifndef Pds_ENTRYDESCScalar_HH
#define Pds_ENTRYDESCScalar_HH

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescScalar : public DescEntry {
  public:
    DescScalar(const char* name, 
	       const char* ytitle);
  };
};

#endif
