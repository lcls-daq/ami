#include "PeakFitPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include "psalg/psalg.h"

#include <QtCore/QString>

#include <stdio.h>

namespace Ami {
  class EntryAccessor {
  public:
    virtual ~EntryAccessor() {}
    virtual double bin_value(unsigned,bool&) const = 0;
    virtual unsigned nbins() const = 0;
    virtual double xlow() const = 0;
    virtual double xup () const = 0;
  };
  class TH1FAccessor : public EntryAccessor {
  public:
    TH1FAccessor(const EntryTH1F& e) : _entry(e) {}
    double bin_value(unsigned bin,
		     bool& valid) const 
    { valid=true; return _entry.content(bin)/_entry.info(EntryTH1F::Normalization); }
    unsigned nbins() const { return _entry.desc().nbins(); }
    double xlow() const { return _entry.desc().xlow(); }
    double xup () const { return _entry.desc().xup (); }
  private:
    const EntryTH1F& _entry;
  };
  class ProfAccessor : public EntryAccessor {
  public:
    ProfAccessor(const EntryProf& e) : _entry(e) {}
    double bin_value(unsigned bin,
		     bool& valid) const 
    {
      double n=_entry.nentries(bin);
      if (!(n>0)) { valid=false; return 0; }
      else { valid = true; return _entry.ysum(bin)/n; } 
    }
    unsigned nbins() const { return _entry.desc().nbins(); }
    double xlow() const { return _entry.desc().xlow(); }
    double xup () const { return _entry.desc().xup (); }
  private:
    const EntryProf& _entry;
  };
};

using namespace Ami;


PeakFitPlot::PeakFitPlot(const DescEntry& output, 
			 double    baseline,
			 Parameter prm) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _nbins     (0),
  _baseline  (baseline),
  _prm       (prm),
  _cache     (0),
  _term      (0),
  _entry     (0)
{
  memcpy (_desc_buffer, &output, output.size());
  memset (_desc_buffer+output.size(), 0, DESC_LEN-output.size());
}

PeakFitPlot::PeakFitPlot(const DescEntry& output, 
			 int nbins,
                         int *bins,
			 Parameter prm) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _nbins     (nbins > MAX_BINS ? MAX_BINS : nbins),
  _baseline  (0),
  _prm       (prm),
  _cache     (0),
  _term      (0),
  _entry     (0),
  _v         (true)
{
  memcpy (_desc_buffer, &output, output.size());
  memcpy (_bins, bins, nbins * sizeof(int));
}

PeakFitPlot::PeakFitPlot(const char*& p, 
                         FeatureCache& features) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache (&features),
  _term  (0),
  _v     (true)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_nbins  , sizeof(_nbins));
  if (_nbins == 0)
      _extract(p, &_baseline  , sizeof(_baseline));
  else {
      for (int i = 0; i < _nbins; i++)
          _extract(p, &_bins[i]  , sizeof(_bins[i]));
  }
  _extract(p, &_prm  , sizeof(_prm  ));

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);

  if (o.type()==DescEntry::Prof ||
      o.type()==DescEntry::Scan ||
      o.type()==DescEntry::TH2F) {
    QString expr(o.xtitle());
    FeatureExpression parser;
    _term = parser.evaluate(features,expr);
    if (!_term) {
      printf("PeakFitPlot failed to parse %s\n",qPrintable(expr));
      _v = false;
    }
  }
}

PeakFitPlot::PeakFitPlot(const char*& p) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache(0),
  _term (0),
  _entry(0),
  _v    (true)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_nbins  , sizeof(_nbins));
  if (_nbins == 0)
      _extract(p, &_baseline  , sizeof(_baseline));
  else {
      for (int i = 0; i < _nbins; i++)
          _extract(p, &_bins[i]  , sizeof(_bins[i]));
  }
  _extract(p, &_prm  , sizeof(_prm  ));
}

PeakFitPlot::~PeakFitPlot()
{
  if (_term ) delete _term;
  if (_entry) delete _entry;
}

void PeakFitPlot::use() { if (_term) _term->use(); }

PeakFitPlot::Parameter  PeakFitPlot::prm      () const { return _prm; }

DescEntry& PeakFitPlot::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

void*      PeakFitPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_nbins  , sizeof(_nbins));
  if (_nbins == 0)
      _insert(p, &_baseline  , sizeof(_baseline));
  else {
      for (int i = 0; i < _nbins; i++)
          _insert(p, &_bins[i]  , sizeof(_bins[i]));
  }
  _insert(p, &_prm  , sizeof(_prm));
  return p;
}

Entry&     PeakFitPlot::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_entry;

  ndarray<const double,1> input;
  ndarray<const double,1> norm;
  double dnorm=1;
  double xscale, xlow;

  switch(e.desc().type()) {
  case DescEntry::TH1F: 
    { const EntryTH1F& entry = static_cast<const EntryTH1F&>(e);
      input  = make_ndarray(entry.content(),entry.desc().nbins());
      dnorm  = entry.info(EntryTH1F::Normalization);
      xscale = (entry.desc().xup()-entry.desc().xlow())/double(entry.desc().nbins());
      xlow   = entry.desc().xlow();
    } break;
  case DescEntry::Prof: 
    { const EntryProf& entry = static_cast<const EntryProf&>(e);
      input  = make_ndarray(entry.ysum()   ,entry.desc().nbins());
      norm   = make_ndarray(entry.entries(),entry.desc().nbins());
      xscale = (entry.desc().xup()-entry.desc().xlow())/double(entry.desc().nbins());
      xlow   = entry.desc().xlow();
    } break;
  default: 
    printf("PeakFit on type %d not implemented\n",e.desc().type());
    return *_entry;
  }

  ndarray<double,1> baseline;
  if (_nbins==0) {
    baseline = make_ndarray<double>(1);
    baseline[0] = _baseline;
  }
  else {
    ndarray<double,1> linput = make_ndarray<double>(_nbins);
    ndarray<unsigned,1> lpos = make_ndarray<unsigned>(_nbins);
    for(int i=0; i<_nbins; i++) {
      linput[i] = input[_bins[i]];
      linput[i] = _bins[i];
    }

    if (norm.empty())
      baseline = psalg::line_fit(linput, lpos, dnorm);
    else {
      ndarray<double,1> lnorm = make_ndarray<double>(_nbins);
      for(int i=0; i<_nbins; i++)
        lnorm[i] = norm[_bins[i]];
      baseline = psalg::line_fit(linput, lpos, lnorm);
    }
  }

  double y = 0;
  if (_prm==RMS) {
    y = norm.empty() ? 
      psalg::dist_rms(input,dnorm,baseline) :
      psalg::dist_rms(input,norm,baseline);
    if (y==0)
      return *_entry;
    y *= xscale;
  }

  else if (_prm==FWHM) {
    y = norm.empty() ?
      psalg::dist_fwhm(input,dnorm,baseline) :
      psalg::dist_fwhm(input,norm,baseline);
    if (y==0)
      return *_entry;
    y *= xscale;
  }

  else { // quadratic interpolation
    
    ndarray<double,1> result = norm.empty() ?
      psalg::parab_interp(input,dnorm,baseline) :
      psalg::parab_interp(input,norm,baseline);
    
    if (result[0]==0 && _prm != Position)
      return *_entry;
    else {  
      if (_prm == Position) y = result[1]*xscale + xlow;
      else                  y = result[0];
    }
  }

  bool damaged=false;
  switch(_entry->desc().type()) {
  case DescEntry::Scalar: 
    { EntryScalar* en = static_cast<EntryScalar*>(_entry);
      en->addcontent(y);    
      break; }
  case DescEntry::ScalarRange: 
    { EntryScalarRange* en = static_cast<EntryScalarRange*>(_entry);
      en->addcontent(y);    
      break; }
  case DescEntry::ScalarDRange: 
    if (!_term) return *_entry;
    { double x=_term->evaluate();
      if (!damaged) {
        EntryScalarDRange* en = static_cast<EntryScalarDRange*>(_entry);
        en->addcontent(x,y);    
      }
      break; }
  case DescEntry::TH1F: 
    { EntryTH1F* en = static_cast<EntryTH1F*>(_entry);
      en->addcontent(1.,y); 
      en->addinfo(1.,EntryTH1F::Normalization);
      break; }
  case DescEntry::TH2F: 
    if (!_term) return *_entry;
    { double x=_term->evaluate();
      if (!damaged) {
        EntryTH2F* en = static_cast<EntryTH2F*>(_entry);
        en->addcontent(1.,x,y); 
        en->addinfo(1.,EntryTH2F::Normalization);
      }
      break; }
  case DescEntry::Prof:    
    if (!_term)
      return *_entry;
    { double x=_term->evaluate();
      if (!damaged) {
	EntryProf* en = static_cast<EntryProf*>(_entry);
	en->addy(y,x);
	en->addinfo(1.,EntryProf::Normalization);
      }
      break; }
  case DescEntry::Scan:    
    if (!_term)
      return *_entry;
    { double x=_term->evaluate();
      if (!damaged) {
	EntryScan* en = static_cast<EntryScan*>(_entry);
	en->addy(y,x,1.,e.last());
	en->addinfo(1.,EntryScan::Normalization);
      }
      break; }
  case DescEntry::Cache:
    { EntryCache* en = static_cast<EntryCache*>(_entry);
      en->set(y,damaged);
      break; }
  case DescEntry::Waveform:
  case DescEntry::Image:
  default:
    printf("PeakFitPlot::_operator no implementation for type %d\n",_entry->desc().type());
    break;
  }
  if (!damaged)
    _entry->valid(e.time());
  return *_entry;
}

const char* PeakFitPlot::name(Parameter p)
{
  static const char* names[] = {"Position", "Height", "FWHM", "RMS", NULL };
  return names[p];
}

void PeakFitPlot::_invalid() {}
