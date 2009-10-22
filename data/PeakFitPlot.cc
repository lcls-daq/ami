#include "PeakFitPlot.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureExpression.hh"

#include <QtCore/QString>

#include <stdio.h>

namespace Ami {
  class EntryAccessor {
  public:
    virtual ~EntryAccessor() {}
    virtual double bin_value(unsigned) const = 0;
    virtual unsigned nbins() const = 0;
    virtual double xlow() const = 0;
    virtual double xup () const = 0;
  };
  class TH1FAccessor : public EntryAccessor {
  public:
    TH1FAccessor(const EntryTH1F& e) : _entry(e) {}
    double bin_value(unsigned bin) const { return _entry.content(bin); }
    unsigned nbins() const { return _entry.desc().nbins(); }
    double xlow() const { return _entry.desc().xlow(); }
    double xup () const { return _entry.desc().xup (); }
  private:
    const EntryTH1F& _entry;
  };
  class ProfAccessor : public EntryAccessor {
  public:
    ProfAccessor(const EntryProf& e) : _entry(e) {}
    double bin_value(unsigned bin) const { return _entry.ymean(bin); }
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
			 Parameter prm,
			 const char* feature) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _baseline  (baseline),
  _prm       (prm),
  _cache     (0),
  _entry     (0)
{
  memcpy (_desc_buffer, &output, output.size());
  strncpy(_feature, feature, FEATURE_LEN);
}

PeakFitPlot::PeakFitPlot(const char*& p, FeatureCache& features) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache (&features)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_baseline  , sizeof(_baseline));
  _extract(p, &_prm  , sizeof(_prm  ));
  _extract(p, _feature , FEATURE_LEN);

  const DescEntry& o = *reinterpret_cast<const DescEntry*>(_desc_buffer);

  _entry = EntryFactory::entry(o);

  QString expr(_feature);
  FeatureExpression parser;
  _term = parser.evaluate(features,expr);
  if (!_term)
    printf("PeakFitPlot failed to parse %s\n",qPrintable(expr));
}

PeakFitPlot::PeakFitPlot(const char*& p) :
  AbsOperator(AbsOperator::PeakFitPlot),
  _cache(0)
{
  _extract(p, _desc_buffer, DESC_LEN);
  _extract(p, &_baseline  , sizeof(_baseline));
  _extract(p, &_prm  , sizeof(_prm  ));
  _extract(p, _feature , FEATURE_LEN);
}

PeakFitPlot::~PeakFitPlot()
{
  if (_term) delete _term;
  if (_entry) delete _entry;
}

PeakFitPlot::Parameter  PeakFitPlot::prm      () const { return _prm; }

DescEntry& PeakFitPlot::output   () const 
{ 
  return _entry ? _entry->desc() : *reinterpret_cast<DescEntry*>(const_cast<char*>(_desc_buffer)); 
}

const char* PeakFitPlot::feature() const { return _feature; }

void*      PeakFitPlot::_serialize(void* p) const
{
  _insert(p, _desc_buffer, DESC_LEN);
  _insert(p, &_baseline  , sizeof(_baseline));
  _insert(p, &_prm  , sizeof(_prm));
  _insert(p, _feature, FEATURE_LEN);
  return p;
}

Entry&     PeakFitPlot::_operate(const Entry& e) const
{
  EntryAccessor* acc;
  switch(e.desc().type()) {
  case DescEntry::TH1F: acc = new TH1FAccessor(static_cast<const EntryTH1F&>(e)); break;
  case DescEntry::Prof: acc = new ProfAccessor(static_cast<const EntryProf&>(e)); break;
  default: 
    printf("PeakFit on type %d not implemented\n",e.desc().type());
    return *_entry;
  }

  double y=0;
  if (_prm==RMS) {
    double x0=0, x1=0, x2=0;
    for(unsigned i=0; i<acc->nbins(); i++) {
      double v  = acc->bin_value(i)-_baseline;
      if (v>0) {
	double vi = v*double(i);
	x0 += v;
	x1 += vi;
	x2 += vi*double(i);
      }
    }
    if (x0>0) {
      x1 /= x0;
      x2 /= x0;
      y = sqrt(x2-x1*x1);
    }
  }
  else {
    unsigned v=0; y=acc->bin_value(0);
    for(unsigned i=1; i<acc->nbins(); i++) {
      double z = acc->bin_value(i);
      if (z>y) { y=z; v=i; }
    }

    if (_prm==FWHM) {  // Find closest edges that fall below half maximum

      double y2 = 0.5*(y+_baseline);
    
      double x0=0;
      { unsigned i=v; 
	while(i!=0) {
	  double z = acc->bin_value(--i);
	  if (z < y2) {
	    x0 = double(i) + (y2-z)/(acc->bin_value(i+1)-z);
	    break;
	  }
	}
      }

      double x1=acc->nbins();
      { unsigned i=v; 
	do {
	  double z = acc->bin_value(++i);
	  if (z < y2) {
	    x1 = double(i) - (y2-z)/(acc->bin_value(i-1)-z);
	    break;
	  }
	} while(i<acc->nbins());
      }
      
      y = (x1-x0)*(acc->xup()-acc->xlow())/double(acc->nbins());
    }

    else {  // quadratic fit

      if (v == 0) {
	if (_prm == Position)
	  y = acc->xlow();
      }
      else if (v == acc->nbins()-1) {
	if (_prm == Position)
	  y = acc->xup();
      }
      else {
	double y1 = acc->bin_value(v-1);
	double y2 = acc->bin_value(v)  ;
	double y3 = acc->bin_value(v+1);
	double a = y2 - 0.5*(y1+y3);
	double b = 0.25*(y3-y1);
	double di = a==0 ? 0 : b/a;
	if (_prm == Position)
	  y = acc->xlow() + (double(v) + di)*(acc->xup()-acc->xlow())/double(acc->nbins());
	else
	  y = y2 + b*di;
      }
      if (_prm == Height)
	y -= _baseline;
    }
  }

  bool damaged=false;
  switch(_entry->desc().type()) {
  case DescEntry::Scalar: 
    { EntryScalar* en = static_cast<EntryScalar*>(_entry);
      en->addcontent(y);    
      break; }
  case DescEntry::TH1F: 
    { EntryTH1F* en = static_cast<EntryTH1F*>(_entry);
      en->addcontent(1.,y); 
      en->addinfo(1.,EntryTH1F::Normalization);
      break; }
  case DescEntry::Prof:    
    { double x=_term->evaluate();
      if (!damaged) {
	EntryProf* en = static_cast<EntryProf*>(_entry);
	en->addy(y,x);
	en->addinfo(1.,EntryProf::Normalization);
      }
      break; }
  case DescEntry::Scan:    
    { double x=_term->evaluate();
      if (!damaged) {
	EntryScan* en = static_cast<EntryScan*>(_entry);
	en->addy(y,x);
	en->addinfo(1.,EntryScan::Normalization);
      }
      break; }
  case DescEntry::Waveform:
  case DescEntry::TH2F:
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
