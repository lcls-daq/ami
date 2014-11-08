#ifndef Ami_LineFit_hh
#define Ami_LineFit_hh

//
//  class LineFit : an operator that fits a line to scalar variables
//    in the event.  The accumulated result is stored in an 
//    Entry{Scalar,Scan,Prof,Prof2D}.
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"

#include <string>

namespace Ami {

  class Cds;
  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;
  class LineFitAcc;

  class LineFit : public AbsOperator, public ScalarPlot {
  public:
    enum Method { LeastSquares, MedianSlope, NumberOf };
    static const char* method_str(Method);
    static Method      method(const char*);

    //  Defined by the output entry's description,
    //    the BLD/PV, and any BLD/PV dependence for profiles.
    LineFit(const DescEntry& output, Method);
    //  Reconstituted from the input serial stream, the BldState and PvState
    //    accessors.
    LineFit(const char*&, 
	    FeatureCache& input);
    ~LineFit();
  public:
    void use();
  public:
    static std::string title(const char* x, const char* y, const char* stat);
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    enum { DESC_LEN = 1024 };
    uint32_t      _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    Method        _method;

    FeatureCache* _cache;
    Entry*        _entry;
    Term*         _xline;
    Term*         _yline;
    bool          _v;

    LineFitAcc*   _acc;  // LeastSquares or WgtMedSlope
  };

};

#endif
