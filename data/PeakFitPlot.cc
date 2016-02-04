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
#include "ami/data/valgnd.hh"
#include "ami/data/FeatureExpression.hh"

#include "psalg/psalg.h"

#include <QtCore/QString>

#include <stdio.h>

using namespace Ami;


PeakFitPlot::PeakFitPlot(const DescEntry& output, 
			 double    baseline,
			 Parameter prm) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _nbins     (0),
  _baseline  (baseline),
  _prm       (prm),
  _cache     (0),
  _entry     (0)
{
  memcpy_val (_desc_buffer, &output, output.size(), DESC_LEN);
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
  _entry     (0),
  _v         (true)
{
  memcpy_val (_desc_buffer, &output, output.size(), DESC_LEN);
  memcpy (_bins, bins, nbins * sizeof(int));
}

PeakFitPlot::PeakFitPlot(const char*& p, 
                         FeatureCache& features,
                         FeatureCache& ocache) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache (&features),
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

  _entry = EntryFactory::entry(o,&ocache);

  _v = _setup(o, features);
}

PeakFitPlot::PeakFitPlot(const char*& p) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache(0),
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
  if (_entry) delete _entry;
}

void PeakFitPlot::use() { _use(); }

PeakFitPlot::Parameter  PeakFitPlot::prm      () const { return _prm; }

const DescEntry& PeakFitPlot::_routput   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<const DescEntry*>(_desc_buffer); 
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
      lpos[i] = _bins[i];
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

  _fill(*_entry, y, e.time());

  return *_entry;
}

const char* PeakFitPlot::name(Parameter p)
{
  static const char* names[] = {"Position", "Height", "FWHM", "RMS", NULL };
  return names[p];
}

void PeakFitPlot::_invalid() {}
