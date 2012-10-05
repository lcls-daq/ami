#ifndef Ami_PeakFinder_hh
#define Ami_PeakFinder_hh

#include "ami/data/AbsOperator.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class EntryImage;
  class PeakFinderFn;

  //
  //  Peak (hit) finder
  //
  class PeakFinder : public AbsOperator {
  public:
    PeakFinder(double threshold_v0,
               double threshold_v1,
               bool   accumulate);
    PeakFinder(const char*&);
    PeakFinder(const char*&, const DescEntry&);
    ~PeakFinder();
  public:
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  public:
    static void register_(unsigned,PeakFinderFn*);
  private:
    double            _threshold_v0;
    double            _threshold_v1;
    bool              _accumulate;
    EntryImage*       _output_entry;
    PeakFinderFn*     _fn;
  };

};

#endif
