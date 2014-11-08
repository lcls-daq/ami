#ifndef Ami_ScalarPlot_hh
#define Ami_ScalarPlot_hh

#include <string>

//
//  class ScalarPlot : a base class for operators that evaluate
//    a scalar quantity.
//

namespace Pds { class ClockTime; }

namespace Ami {

  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;

  class ScalarPlot {
  public:
    ScalarPlot();
    virtual ~ScalarPlot();
  private:
    virtual Term* _process_expr(FeatureCache&,const char*,bool&);
  protected:
    bool _setup(const DescEntry&, FeatureCache& input);
    void _use  ();
    void _fill (Entry&, double, const Pds::ClockTime&,
		bool initial=true) const;
  protected:
    Term*         _xterm;
    Term*         _yterm;
    Term*         _weight;
    mutable double _x, _y, _w;
    bool   _xterm_uses, _yterm_uses, _weight_uses;
  };

};

#endif
