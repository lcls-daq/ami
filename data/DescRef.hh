#ifndef Pds_DescRef_hh
#define Pds_DescRef_hh

#include "ami/data/DescEntry.hh"

namespace Ami {

  class DescRef : public DescEntry {
  public:
    DescRef(const char* name);

    DescRef(const Pds::DetInfo& info,
	    unsigned channel,
	    const char* name);
  };
};

#endif
