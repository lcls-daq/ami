#ifndef Pds_ENTRYScalarRange_HH
#define Pds_ENTRYScalarRange_HH

#include "ami/data/Entry.hh"
#include "ami/data/DescScalarRange.hh"

#include <math.h>

namespace Ami {

  class DescTH1F;

  class EntryScalarRange : public Entry {
  public:
    EntryScalarRange(const DescScalarRange& desc);

    virtual ~EntryScalarRange();

    //  The contents are organized as 
    double entries() const;
    double sum    () const;
    double sqsum  () const;
    double mean   () const;
    double rms    () const;
    double min    () const;
    double max    () const;
    void   addcontent(double y);

    void setto(const EntryScalarRange& entry);
    void add  (const EntryScalarRange& entry);

    // Implements Entry
    virtual const DescScalarRange& desc() const;
    virtual DescScalarRange& desc();

    DescTH1F* result(void* =0) const;
  private:
    DescScalarRange _desc;

  private:
    double* _y;
  };

  inline double EntryScalarRange::entries() const { return _y[0]; }
  inline double EntryScalarRange::sum    () const { return _y[1]; }
  inline double EntryScalarRange::sqsum  () const { return _y[2]; }
  inline double EntryScalarRange::mean   () const { return _y[0] ? _y[1]/_y[0] : 0; }
  inline double EntryScalarRange::rms    () const { return _y[0] ? sqrt((_y[2] - _y[1]*_y[1]/_y[0])/_y[0]) : 0; }
  inline double EntryScalarRange::min    () const { return _y[3]; }
  inline double EntryScalarRange::max    () const { return _y[4]; }
  inline void   EntryScalarRange::addcontent(double y) { _y[0]++; _y[1]+=y; _y[2]+=y*y; if (y<_y[3]) _y[3]=y; if (y>_y[4]) _y[4]=y; }
};

#endif
