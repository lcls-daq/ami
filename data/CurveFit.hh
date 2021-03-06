#ifndef Ami_CurveFit_hh
#define Ami_CurveFit_hh

#include "ami/data/AbsOperator.hh"
#include "ami/data/ScalarPlot.hh"
#include "ami/data/DescTH1F.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include <vector>

namespace Ami {

  class DescEntry;
  class EntryWaveform;
  class EntryTH1F;
  class Term;
  class FeatureCache;

  //
  //  Leading edge finder
  //
  class CurveFit : public AbsOperator, public ScalarPlot {
  public:
    CurveFit(const char *name, int op, const DescEntry& output, const char *norm = 0);
    CurveFit(const char*&, const DescEntry& input, FeatureCache& features);
    ~CurveFit();
    char *name() { return _name; };
    char *norm() { return _norm; };
    int   op()   { return _op; };
    void use();
  private:
    const DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return _v; }
    void       _invalid  ();
  private:
    const DescEntry *_input;
    enum { NAME_LEN = 256 };
    char       _name[NAME_LEN];
    enum { DESC_LEN = 1024 };
    uint32_t    _desc_buffer[DESC_LEN/sizeof(uint32_t)];
    enum { scale=0, shift=1, chi2=2 };
    int        _op;
    enum { NORM_LEN = 256 };
    char       _norm[NORM_LEN];
    Entry*     _entry;
    std::vector<double> _refdata;
    double         _refstart, _refend;
    double         _refbw;
    Term*         _nterm;
    bool          _v;

  public:
    enum { CALC_LEN = 10 };
    static struct recent {
        const DescEntry *input;
        Pds::ClockTime   time;
        double vals[3];
    } calc[CALC_LEN];
    static int calcnxt;
  };
};

#endif
