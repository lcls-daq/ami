#ifndef Ami_PeakFitPlot_hh
#define Ami_PeakFitPlot_hh

//
//  class PeakFitPlot : an operator that extracts peak parameters from a profile
//    and generates a mean value (Scalar), a distribution (TH1F), or a profile (Prof)
//    versus a BLD or PV quantity.
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"

namespace Ami {

  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  class PeakFitPlot : public AbsOperator, public ScalarPlot {
  public:
    enum Parameter { Position, Height, FWHM, RMS, NumberOf };
    static const char* name(Parameter);
  public:
    //  Defined by the output entry's description,
    //    the BLD/PV, and any BLD/PV dependence for profiles.
    PeakFitPlot(const DescEntry& output,
		double    baseline,
		Parameter prm);
    PeakFitPlot(const DescEntry& output,
		int       nbins,
                int      *bins,
		Parameter prm);
    //  Reconstituted from the input serial stream, the BldState and PvState
    //    accessors, and the Cds input entry accessor.
    PeakFitPlot(const char*&, FeatureCache&, FeatureCache&);
    PeakFitPlot(const char*&);
    ~PeakFitPlot();
  public:
    unsigned   input    () const;
    Parameter  prm      () const;
    const char* feature() const;
    void       use();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { DESC_LEN = 1024, MAX_BINS = 10 };
    uint32_t         _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    int              _nbins;
    int              _bins[MAX_BINS];
    double           _baseline;
    Parameter        _prm;

    FeatureCache* _cache;
    Entry*        _entry;
    bool          _v;
  };

};

#endif
