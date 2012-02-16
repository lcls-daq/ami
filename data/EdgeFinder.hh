#ifndef Ami_EdgeFinder_hh
#define Ami_EdgeFinder_hh

#include "ami/data/AbsOperator.hh"

#include "ami/data/DescTH1F.hh"

namespace Ami {

  class DescEntry;
  class EntryWaveform;
  class EntryTH1F;

  //
  //  Edge finder
  //
  class EdgeFinder : public AbsOperator {
  public:
    enum Algorithm {
                     halfbase2peak = 0,
                   };
    inline static int  EdgeAlgorithm(Algorithm a, bool leading)
      { return 2 * a + (leading ? 0 : 1); };
    inline static bool IsLeading(int a)
      { return (a & 1) ? false : true; };

    EdgeFinder(double     fraction,
	       double     threshold_value,
	       double     baseline_value,
               int        alg,
	       const      DescTH1F& output);
    EdgeFinder(const char*&);
    EdgeFinder(double fraction, int alg, const char*&);
    ~EdgeFinder();
  public:
    inline double threshold() { return _threshold_value; }
    inline double baseline()  { return _baseline_value; }
    inline int    algorithm() { return _alg; }
    inline double fraction()  { return _fraction; }
    DescEntry& output   () const;
    void*      desc   () const;
    int        desc_size () const;
  private:
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
  private:
    double     _fraction;
    int        _alg;
    double     _threshold_value;
    double     _baseline_value;
    DescTH1F   _output;
    EntryTH1F*           _output_entry;
  };

};

#endif
