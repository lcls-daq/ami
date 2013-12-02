#ifndef Ami_Variance_hh
#define Ami_Variance_hh

#include "ami/data/AbsOperator.hh"

#include "ndarray/ndarray.h"
#include <vector>

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class FeatureCache;
  class Term;

  /**
   *   An operator to compute the root-mean-square variance of each element
   *   of the data.  Thus, this operator acting on a waveform produces a waveform
   *   where each sample is replaced by the RMS of that sample.  The variance is
   *   calculated over either a fixed number of events or a running calculation over
   *   all events.
   */
  class Variance : public AbsOperator {
  public:
    Variance(unsigned n=0, const char* p=0);
    Variance(const char*&, const DescEntry&, FeatureCache&);
    ~Variance();
  public:
    void use();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
  private:
    enum { SCALE_LEN=256 };
    char           _scale_buffer[SCALE_LEN];
    unsigned       _n;
    mutable unsigned _i;
    mutable std::vector<ndarray<double,2> > _m1;
    mutable std::vector<ndarray<double,2> > _m2;
    mutable Entry*         _cache;
    mutable const Entry*   _input;
    Term*          _term ;
    bool           _v;
  };

};

#endif
