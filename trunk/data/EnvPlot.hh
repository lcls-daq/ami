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
    //    accessors
    EnvPlot(const char*&, 
	    FeatureCache& input, 
	    FeatureCache& output);
    EnvPlot(const char*&);
    ~EnvPlot();
  public:
    void use();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { DESC_LEN = 1024 };
    uint32_t      _desc_buffer[DESC_LEN/sizeof(uint32_t)];

    Entry*        _entry;
    Term*         _input;
    bool          _v;
  };

};

#endif
