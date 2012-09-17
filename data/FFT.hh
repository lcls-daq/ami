#ifndef Ami_FFT_hh
#define Ami_FFT_hh

//
//  class FFT : an operator that applies a 1D Fourier transform
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescProf.hh"

namespace Ami {

  class DescEntry;
  class Entry;

  class FFT : public AbsOperator {
  public:
    FFT();
    FFT(const char*&, const DescEntry&);
    ~FFT();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
  private:
    Entry*           _output;
  };

};

#endif
