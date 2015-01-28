#include "FFT.hh"

#include "ami/data/Complex.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescWaveform.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"

#include <ndarray/ndarray.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>

#include <stdio.h>
#include <math.h>

using namespace Ami;

FFT::FFT(Parameter p) : 
  AbsOperator(AbsOperator::FFT),
  _parameter (p),
  _output    (0),
  _wavetable (0),
  _workspace (0),
  _complex   (0)
{
}

FFT::FFT(const char*& p) :
  AbsOperator(AbsOperator::FFT),
  _output    (0),
  _wavetable (0),
  _workspace (0),
  _complex   (0)
{
  _extract(p, &_parameter, sizeof(unsigned));
}

FFT::FFT(const char*& p, const DescEntry& input) :
  AbsOperator(AbsOperator::FFT)
{
  _extract(p, &_parameter, sizeof(unsigned));

  unsigned n=0;

  switch(input.type()) {
  case DescEntry::TH1F:
    { const DescTH1F& inputd = static_cast<const DescTH1F&>(input);
      float dw = 1./(inputd.xup()-inputd.xlow());
      n = inputd.nbins();
      DescTH1F desc(inputd.info(), inputd.channel(),
		    inputd.name(), inputd.xtitle(), inputd.ytitle(),
		    (n+1)/2, -0.5*dw, dw*(float((n+1)/2)+0.5));
      _output = new EntryTH1F(desc);
      break; }
  case DescEntry::Waveform:
    { const DescWaveform& inputd = static_cast<const DescWaveform&>(input);
      float dw = 1./(inputd.xup()-inputd.xlow());
      n = inputd.nbins();
      DescWaveform desc(inputd.info(), inputd.channel(),
			inputd.name(), inputd.xtitle(), inputd.ytitle(),
			(n+1)/2, -0.5*dw, dw*(float((n+1)/2)+0.5));
      _output = new EntryWaveform(desc);
      break; }
  default:
    _output = 0;
    printf("FFT of type %d not supported\n",input.type());
    break;
  }

  _workspace = gsl_fft_real_workspace_alloc(n);
  _wavetable = gsl_fft_real_wavetable_alloc(n);
  _complex   = gsl_vector_complex_alloc(n);
}

FFT::~FFT()
{
  if (_output   ) delete _output;
  if (_wavetable) gsl_fft_real_wavetable_free(_wavetable);
  if (_workspace) gsl_fft_real_workspace_free(_workspace);
  if (_complex  ) gsl_vector_complex_free(_complex);
}

const DescEntry& FFT::_routput   () const 
{ 
  return _output->desc();
}

void*      FFT::_serialize(void* p) const
{
  _insert(p, &_parameter, sizeof(unsigned));
  return p;
}

Entry&     FFT::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  unsigned na(0);
  ndarray<double,1> a;

#define CASETERM(type) case DescEntry::type: {				\
    const Entry##type& en = static_cast<const Entry##type&>(e);		\
    const Desc##type& d = static_cast<const Desc##type&>(e.desc());	\
    na = d.nbins();							\
    double n = en.info(Entry##type::Normalization);			\
    n = (n==0) ? 1:1./n;						\
    a = make_ndarray<double>(na);					\
    for(unsigned i=0; i<na; i++)					\
      a[i] = en.content(i)*n;						\
  } break;
    
  switch(output().type()) {
    CASETERM(TH1F)
    CASETERM(Waveform)
      default: break;
  }

#undef CASETERM

  int result = gsl_fft_real_transform(a.data(), 1, na, _wavetable, _workspace);
  if (result==GSL_SUCCESS) {
    gsl_complex* c = (gsl_complex*)(&a[-1]);

#define CASETERM(type) case DescEntry::type: {				\
      Entry##type& _e = *static_cast<Entry##type*>(_output);		\
      _e.info(1.,Entry##type::Normalization);                           \
      switch(_parameter) {						\
      case Ampl:							\
	_e.content(a[0],0);						\
	for(unsigned i=1; i<(na+1)/2; i++)				\
	  _e.content(gsl_complex_abs(c[i]),i);				\
	break;								\
      case Phase:							\
	_e.content(0.,0);						\
	for(unsigned i=1; i<(na+1)/2; i++)				\
	  _e.content(gsl_complex_arg(c[i]),i);				\
	break;								\
      case Real:							\
	_e.content(a[0],0);						\
	for(unsigned i=1; i<(na+1)/2; i++)				\
	  _e.content(GSL_REAL(c[i]),i);					\
	break;								\
      case Imag:							\
	_e.content(0.,0);						\
	for(unsigned i=1; i<(na+1)/2; i++)				\
	  _e.content(GSL_IMAG(c[i]),i);					\
	break;								\
      default: break;							\
      } } break;

    switch(e.desc().type()) {
    CASETERM(TH1F)
    CASETERM(Waveform)
      default: break;
    }

#undef CASETERM

    _output->valid(e.time());
  }
  else {
    printf("FFT:gsl error %d\n",result);
    _output->invalid();
  }

  return *_output;
}

static const char* parameters[] = { "Amplitude", "Phase", "Real", "Imag" };

const char* FFT::parameter(Parameter p)
{
  return p < NumberOf ? parameters[p] : "-Invalid-";
}

void FFT::_invalid() { _output->invalid(); }
