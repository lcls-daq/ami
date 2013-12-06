#ifndef Ami_FFT_hh
#define Ami_FFT_hh

//
//  class FFT : an operator that applies a 1D Fourier transform
//

#include "ami/data/AbsOperator.hh"
#include "ami/data/DescProf.hh"

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_vector.h>

namespace Ami {

  class DescEntry;
  class Entry;

  class FFT : public AbsOperator {
  public:
    enum Parameter { Ampl, Phase, Real, Imag, NumberOf };
    static const char* parameter(Parameter);
  public:
    FFT(Parameter);
    FFT(const char*&);
    FFT(const char*&, const DescEntry&);
    ~FFT();
  private:
    DescEntry& _routput   () const;
    Entry&     _operate  (const Entry&) const;
    void*      _serialize(void*) const;
    bool       _valid    () const { return true; }
    void       _invalid  ();
  private:
    Parameter        _parameter;
    Entry*           _output;
    gsl_fft_real_wavetable* _wavetable;
    gsl_fft_real_workspace* _workspace;
    gsl_vector_complex*     _complex;
  };

};

#endif
