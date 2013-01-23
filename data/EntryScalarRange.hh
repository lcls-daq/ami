#ifndef Pds_ENTRYScalarRange_HH
#define Pds_ENTRYScalarRange_HH

#include "ami/data/EntryAutoRange.hh"
#include "ami/data/DescScalarRange.hh"
#include "ami/data/ScalarRange.hh"
#include "ami/data/DescTH1F.hh"

#include <math.h>

namespace Ami {

  class EntryScalarRange : public EntryAutoRange {
  public:
    EntryScalarRange(const DescScalarRange& desc);

    virtual ~EntryScalarRange();

    void   addcontent(double y);

    void setto(const EntryScalarRange& entry);
    void add  (const EntryScalarRange& entry);

    // Implements Entry
    virtual const DescScalarRange& desc() const;
    virtual DescScalarRange& desc();

    // Implements EntryAutoRange
    double    entries() const;
    DescTH1F* result(void* =0) const;

    const ScalarRange& range() const;
  private:
    virtual void _merge(char*) const;

  private:
    DescScalarRange _desc;

  private:
    ScalarRange* _range;
  };

  inline void   EntryScalarRange::addcontent(double y) { _range->addcontent(y); }
  inline void   EntryScalarRange::setto(const EntryScalarRange& entry) { _range->setto(*entry._range); }
  inline void   EntryScalarRange::add  (const EntryScalarRange& entry) { valid(entry.time()); _range->add(*entry._range); }
  inline void   EntryScalarRange::_merge(char* p) const { _range->merge(p); }

  inline const ScalarRange& EntryScalarRange::range() const { return *_range; }
};

#endif
