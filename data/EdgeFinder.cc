#include "EdgeFinder.hh"

#include "ami/data/FeatureCache.hh"
#include "ami/data/DescRef.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"
#include "ami/data/XML.hh"

#include "ndarray/ndarray.h"
#include "pdsalg/pdsalg.h"

#include <QtCore/QString>

#include <stdio.h>

typedef std::list<ndarray<double,1> > PLIST;

static const unsigned STEP_SIZE = 50;

static PLIST find_edges(const Ami::EntryWaveform& e,
			const Ami::EdgeFinderConfig&,
			Ami::FeatureCache&, unsigned index);

static void _add_edge(const double*,
		      double                  sc,
		      const Ami::EdgeFinderConfig& c,
		      const Ami::DescWaveform&     d,
		      double                  peak, 
		      unsigned                start, 
		      double&                 last,
		      PLIST&                  result);

using namespace Ami;
using Ami::XML::QtPersistent;

static const char* _name[] = { "Ampl", 
			       "Time",
			       "Nedges" };

const char* Edges::name(Edges::Parameter p) { return p <= NumberOf ? _name[p] : "-Invalid-"; }


EdgeFinder::EdgeFinder(const char* name,
		       const EdgeFinderConfig& config) :
  AbsOperator(AbsOperator::EdgeFinder),
  _config    (config),
  _entry     (0)
{
  memcpy (_name, name, NAME_LEN);
}

#define CASETERM(type) case DescEntry::type: \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

EdgeFinder::EdgeFinder(const char*& p,
		       FeatureCache& cache) :
  AbsOperator     (AbsOperator::EdgeFinder),
  _v              (true),
  _cache          (&cache)
{
  _extract(p, _name   , NAME_LEN);
  _extract(p, &_config, sizeof(_config));

  DescRef o(_name);
  _entry = new EntryRef(o);
  _entry->set(&_output);
}

EdgeFinder::~EdgeFinder()
{
  if (_entry) delete _entry;
}

void EdgeFinder::use() {}

DescEntry& EdgeFinder::_routput   () const 
{ 
  if (_entry) return _entry->desc();
  abort();
}

void*      EdgeFinder::_serialize(void* p) const
{
  _insert(p, _name   , NAME_LEN);
  _insert(p, &_config, sizeof(_config));
  return p;
}

Entry&     EdgeFinder::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_entry;

  const EntryWaveform& entry = static_cast<const EntryWaveform&>(e);

  Edges& output = const_cast<EdgeFinder*>(this)->_output;
  output.reset();

  PLIST edges;
  edges = find_edges(entry, _config, *_cache, _index);
  
  const unsigned n = edges.size();
#ifdef DBUG
  printf("EdgeFinder::edges %d\n",n);
#endif
  if (n) {
    output.resize(n);
    PLIST::iterator it=edges.begin();
    for(unsigned i=0; i<n; i++,it++)
      output.append(it->data());
  }
  _entry->valid(e.time());
  return *_entry;
}

PLIST find_edges(const EntryWaveform& e,
		 const EdgeFinderConfig& c,
		 FeatureCache& cache,
		 unsigned index)
{
  PLIST result;

  const double* v = e.content();
  const DescWaveform& d = e.desc();
  double sc=e.info(EntryWaveform::Normalization);
  if (sc==0) sc=1;

  double threshold_value = c._threshold_value*sc;
  double   peak   = threshold_value;
  unsigned start  = 0;
  double   last   = -1.0;
  bool     crossed=false;
  bool     rising = c._threshold_value > c._baseline_value;
  for(unsigned k=0; k<d.nbins(); k++) {
    double y = v[k];
    bool over = 
      ( rising && y>threshold_value) ||
      (!rising && y<threshold_value);
    if (!crossed && over) {
      crossed = true;
      start   = k;
      peak    = y;
    }
    else if (crossed && !over) {
      _add_edge(v, sc, c, d, peak, start, last, result);
      crossed = false;
    }
    else if (( rising && y>peak) ||
	     (!rising && y<peak)) {
      peak = y;
      if (!c._leading_edge)  // For a trailing edge, start at the peak!
        start = k;
    }
  }

  //  The last edge may not have fallen back below threshold
  if (crossed) {
    _add_edge(v, sc, c, d, peak, start, last, result);
  }

  return result;
}

void _add_edge(const double*           v,
	       double                  sc,
	       const EdgeFinderConfig& c,
	       const DescWaveform&     d,
	       double                  peak, 
	       unsigned                start, 
	       double&                 last,
	       PLIST&                  result)
{
  double baseline_value  = c._baseline_value*sc;
  bool     rising = c._threshold_value > c._baseline_value;

  //  find the edge
  double edge_v = c._fraction*(peak+baseline_value);
  unsigned i=start;
  if (rising == c._leading_edge) { // leading positive edge, or trailing negative edge
    while(v[i] < edge_v)
      i++;
  }
  else {                           // trailing positive edge, or leading negative edge
    while(v[i] > edge_v)
      i++;
  }
  double edge = i>0 ? 
    (edge_v-v[i])/(v[i]-v[i-1])
    + double(i) : 0;
  double thisx = edge*(d.xup()-d.xlow())/double(d.nbins())+d.xlow();
  if (last < 0 || thisx > last + c._deadtime) {
    ndarray<double,1> a = make_ndarray<double>(Edges::NumberOf);
    a[0] = peak/sc;
    a[1] = thisx;
    result.push_back(a);
    last = thisx;
  }
}

#if 0
Entry&     EdgeFinder::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output_entry;

  const EntryWaveform& entry = static_cast<const EntryWaveform&>(e);
  const DescWaveform& d = entry.desc();
  double sc = entry.info(EntryWaveform::Normalization);
  if (sc==0) sc=1;
  double threshold_value = _threshold_value*sc;

  double xscale = (d.xup()-d.xlow())/double(d.nbins());

  ndarray<const double,1> wf = make_ndarray(entry.content(),d.nbins());
  ndarray<double,2> edges = pdsalg::find_edges(wf,
                                               _baseline_value,
                                               threshold_value,
                                               _fraction,
                                               _deadtime/xscale,
                                               IsLeading(_alg));

  switch(_parameter) {
  case Location:
    { EntryTH1F& e = *static_cast<EntryTH1F*>(_output_entry);
      for(unsigned k=0; k<edges.shape()[0]; k++)
        e.addcontent(1.,xscale*edges[k][0]+d.xlow());
    } break;
  case Amplitude:
    { EntryTH1F& e = *static_cast<EntryTH1F*>(_output_entry);
      for(unsigned k=0; k<edges.shape()[0]; k++)
        e.addcontent(1.,edges[k][1]);
    } break;
  default:
    if (_output->type()==DescEntry::TH2F) {
      EntryTH2F& e = *static_cast<EntryTH2F*>(_output_entry);
      for(unsigned k=0; k<edges.shape()[0]; k++)
        e.addcontent(1.,xscale*edges[k][0]+d.xlow(),edges[k][1]);
    }
    else
      ;
    break;
  }
  
  switch(_output->type()) {
  case DescEntry::TH1F:
    static_cast<EntryTH1F*>(_output_entry)->addinfo(entry.info(EntryWaveform::Normalization),
                                                    EntryTH1F::Normalization);
    break;
  case DescEntry::TH2F:
    static_cast<EntryTH2F*>(_output_entry)->addinfo(entry.info(EntryWaveform::Normalization),
                                                    EntryTH2F::Normalization);
    break;
  default:
    break;
  }
  _output_entry->valid(e.time());
  return *_output_entry;
}

Entry&     EdgeFinder::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output_entry;

  const EntryWaveform& entry = static_cast<const EntryWaveform&>(e);
  const DescWaveform& d = entry.desc();
  // find the boundaries where the pulse crosses the threshold
  double sc = entry.info(EntryWaveform::Normalization);
  if (sc==0) sc=1;
  double threshold_value = _threshold_value*sc;

  double   peak   = threshold_value;
  unsigned start  = 0;
  double   last   = -1.0;
  bool     crossed=false;
  bool     rising = _threshold_value > _baseline_value;
  for(unsigned k=0; k<d.nbins(); k++) {
    double y = entry.content(k);
    bool over = 
      ( rising && y>threshold_value) ||
      (!rising && y<threshold_value);
    if (!crossed && over) {
      crossed = true;
      start   = k;
      peak    = y;
    }
    else if (crossed && !over) {
      _hist_edge(peak, start, last, entry);
      crossed = false;
    }
    else if (( rising && y>peak) ||
	     (!rising && y<peak)) {
      peak = y;
      if (!IsLeading(_alg))  // For a trailing edge, start at the peak!
        start = k;
    }
  }

  //  The last edge may not have fallen back below threshold
  if (crossed) {
    _hist_edge(peak, start, last, entry);
  }

  switch(_output->type()) {
  case DescEntry::TH1F:
    static_cast<EntryTH1F*>(_output_entry)->addinfo(entry.info(EntryWaveform::Normalization),
                                                    EntryTH1F::Normalization);
    break;
  case DescEntry::TH2F:
    static_cast<EntryTH2F*>(_output_entry)->addinfo(entry.info(EntryWaveform::Normalization),
                                                    EntryTH2F::Normalization);
    break;
  default:
    break;
  }
  _output_entry->valid(e.time());
  return *_output_entry;
}


void EdgeFinder::_hist_edge(double               peak, 
			    unsigned             start, 
			    double&              last,
			    const EntryWaveform& entry) const
{
  const DescWaveform& d = entry.desc();
  double sc = entry.info(EntryWaveform::Normalization);
  if (sc==0) sc=1;
  double baseline_value  = _baseline_value*sc;
  bool     rising = _threshold_value > _baseline_value;

  //  find the edge
  double edge_v = _fraction*(peak+baseline_value);
  unsigned i=start;
  if (rising == IsLeading(_alg)) { // leading positive edge, or trailing negative edge
    while(entry.content(i) < edge_v)
      i++;
  }
  else {                           // trailing positive edge, or leading negative edge
    while(entry.content(i) > edge_v)
      i++;
  }
  double edge = i>0 ? 
    (edge_v-entry.content(i))/(entry.content(i)-entry.content(i-1)) 
    + double(i) : 0;
  double thisx = edge*(d.xup()-d.xlow())/double(d.nbins())+d.xlow();
  if (last < 0 || thisx > last + _deadtime) {
    switch(_parameter) {
    case Location:
      static_cast<EntryTH1F*>(_output_entry)->addcontent(1.,thisx);
      break;
    case Amplitude:
      static_cast<EntryTH1F*>(_output_entry)->addcontent(1.,peak);
      break;
    default:
      if (_output->type()==DescEntry::TH2F)
	static_cast<EntryTH2F*>(_output_entry)->addcontent(1.,thisx,peak);
      else
	;
      break;
    }
    last = thisx;
  }
}
#endif

void EdgeFinder::_invalid() {}

void EdgeFinderConfig::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_fraction") 
      _fraction = QtPersistent::extract_d(p);
    else if (tag.name == "_leading_edge") 
      _leading_edge = QtPersistent::extract_b(p);
    else if (tag.name == "_deadtime") 
      _deadtime = QtPersistent::extract_d(p);
    else if (tag.name == "_threshold_value") 
      _threshold_value = QtPersistent::extract_d(p);
    else if (tag.name == "_baseline_value") 
      _baseline_value = QtPersistent::extract_d(p);
  XML_iterate_close(EdgeFinderConfig,tag);
}

void EdgeFinderConfig::save(char*& p) const
{
  XML_insert(p, "double", "_fraction"    , QtPersistent::insert(p,_fraction));
  XML_insert(p, "bool"  , "_leading_edge", QtPersistent::insert(p,_leading_edge));
  XML_insert(p, "double", "_deadtime"    , QtPersistent::insert(p,_deadtime));
  XML_insert(p, "double", "_threshold_value", QtPersistent::insert(p,_threshold_value));
  XML_insert(p, "double", "_baseline_value" , QtPersistent::insert(p,_baseline_value));
}

