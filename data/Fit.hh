#ifndef Ami_Fit_hh
#define Ami_Fit_hh

//
//  class Fit
//
#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"

namespace Ami {

  class DescEntry;
  class Entry;
  class Term;
  class FeatureCache;
  class FitEntry;

  class Fit : public AbsOperator, public ScalarPlot {
  public:
    enum Function { Gauss, Lorentz, NumberOf };
    static const char* function_str(Function);
    static Function    function(const char*);

    Fit(const DescEntry& output, Function, unsigned parameter, double xlo=0, double xhi=0);
    Fit(const DescEntry& output, const Fit&);
    Fit(const char*&, FeatureCache& input, FeatureCache& output);
    Fit(const char*&);
    ~Fit();
  public:
    unsigned    input() const;
    Function    function() const;
    unsigned    parameter() const;
    void        use();
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
    Function      _function;
    unsigned      _parameter;
    double        _xlo;
    double        _xhi;

    FeatureCache* _cache;
    Entry*        _entry;
    bool          _v;

    FitEntry*     _fit;
  };

};

#endif
