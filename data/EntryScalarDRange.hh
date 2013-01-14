#ifndef Pds_ENTRYScalarDRange_HH
#define Pds_ENTRYScalarDRange_HH

#include "ami/data/EntryAutoRange.hh"
#include "ami/data/DescScalarDRange.hh"
#include "ami/data/ScalarRange.hh"
#include "ami/data/DescTH2F.hh"

#include <math.h>

namespace Ami {

  class DescTH2F;

  class EntryScalarDRange : public EntryAutoRange {
  public:
    EntryScalarDRange(const DescScalarDRange& desc);

    virtual ~EntryScalarDRange();

    //  The contents are organized as 
    void   addcontent(double x,double y);

    void setto(const EntryScalarDRange& entry);
    void add  (const EntryScalarDRange& entry);

    // Implements Entry
    virtual const DescScalarDRange& desc() const;
    virtual DescScalarDRange& desc();

    // Implements EntryAutoRange
    double    entries() const;
    DescTH2F* result(void* =0) const;

  private:
    virtual void _merge(char*) const;

  private:
    DescScalarDRange _desc;

  private:
    ScalarRange* _x;
    ScalarRange* _y;
  };

  inline void   EntryScalarDRange::addcontent(double x,double y) { _x->addcontent(x); _y->addcontent(y); }
  inline void   EntryScalarDRange::setto(const EntryScalarDRange& entry) 
  { _x->setto(*entry._x); _y->setto(*entry._y); }
  inline void   EntryScalarDRange::add  (const EntryScalarDRange& entry) 
  { valid(entry.time()); _x->add  (*entry._x); _y->add  (*entry._y); }
  inline void   EntryScalarDRange::_merge(char* p) const
  { _x->merge(p); _y->merge(p+sizeof(ScalarRange)); }
};

#endif
