#ifndef Ami_EnvPlot_hh
#define Ami_EnvPlot_hh

//
//  class EnvPlot : an operator that fetches a BLD or PV quantity
//    and generates a mean value (Scalar), a distribution (TH1F), or a profile (Prof)
//    versus a BLD or PV quantity.
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  class EnvPlot : public AbsOperator, public ScalarPlot {
  public:
    //  Defined by the output entry's description,
    //    the BLD/PV, and any BLD/PV dependence for profiles.
    EnvPlot(const DescEntry& output);
    //  Reconstituted from the input serial stream, the BldState and PvState
    //    accessors, and the Cds input entry accessor.
    EnvPlot(const char*&, 
	    FeatureCache& input, 
	    FeatureCache& output, 
	    const Cds&);
    ~EnvPlot();
  public:
    void use();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { DESC_LEN = 1024 };
    char             _desc_buffer[DESC_LEN];

    FeatureCache* _cache;
    Entry*        _entry;
    Term*         _input;
    bool          _v;
  };

};

#endif
