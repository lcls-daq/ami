#ifndef Ami_PeakFinder_hh
#define Ami_PeakFinder_hh

#include "ami/data/AbsOperator.hh"
#include "ndarray/ndarray.h"

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
    enum Mode   { Count, Sum };
    enum Option { Single, AutoRefresh };
    PeakFinder(double threshold_v0,
               double threshold_v1,
               Mode   mode,
               bool   center_only,
               Option);
    PeakFinder(double threshold_v0,
               double threshold_v1,
               Mode   mode,
               bool   center_only,
               int    accumulate);
    PeakFinder(const char*&);
    PeakFinder(const char*&, const DescEntry&);
    ~PeakFinder();
  public:
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  ();
  public:
    static void register_(unsigned,PeakFinderFn*);
  private:
    double            _threshold_v0;
    double            _threshold_v1;
    Mode              _mode;
    bool              _center_only;
    int               _accumulate;   /// auto refresh (-2), single event (-1), running sum(0), fixed event sum(>0)
    mutable int       _current;
    EntryImage*       _output_entry; /// Holds single event result and indefinite running sum
    EntryImage*       _cache;        /// Holds fixed event number sum
    PeakFinderFn*     _fn;
    ndarray<unsigned,2> _threshold;
  };

};

#endif
