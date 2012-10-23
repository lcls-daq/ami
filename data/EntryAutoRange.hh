#ifndef Pds_EntryAutoRange_HH
#define Pds_EntryAutoRange_HH

#include "ami/data/Entry.hh"

namespace Ami {

  class EntryAutoRange : public Entry {
  public:
    EntryAutoRange() {}
    virtual ~EntryAutoRange() {}

    virtual double     entries() const = 0;
    virtual DescEntry* result(void* =0) const = 0;
  };
};

#endif
