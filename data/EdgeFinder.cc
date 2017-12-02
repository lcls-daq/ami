#include "EdgeFinder.hh"

#include "ami/data/DescRef.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/ImageMask.hh"

#include "ami/data/Cds.hh"
#include "ami/data/XML.hh"

#include "ndarray/ndarray.h"
#include "psalg/psalg.h"

#include <QtCore/QString>

#include <stdio.h>
#include <float.h>

//#define DBUG

typedef std::list<ndarray<double,1> > PLIST;

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

EdgeFinder::EdgeFinder(const char*& p) :
  AbsOperator     (AbsOperator::EdgeFinder),
  _v              (true)
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

const DescEntry& EdgeFinder::_routput   () const 
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

  const DescWaveform& d = entry.desc();
  double sc=entry.info(EntryWaveform::Normalization);
  if (sc==0) sc=1;
  double xsc = (d.xup()-d.xlow())/double(d.nbins());

  unsigned xlo(0);
  if (_config._xlo > d.xlow())
    xlo = unsigned((_config._xlo-d.xlow())*double(d.nbins())/(d.xup()-d.xlow())+0.5);
  
  unsigned xhi(d.nbins());
  if (_config._xhi < d.xup())
    xhi = unsigned((_config._xhi-d.xlow())*double(d.nbins())/(d.xup()-d.xlow())+0.5);

  unsigned nx = xhi-xlo;

#ifdef DBUG
  printf("find_edges: baseline %f  threshold %f  fraction %f  deadtime %u  leadingedge %c\n",
         _config._baseline_value*sc,
         _config._threshold_value*sc,
         _config._fraction,
         unsigned(_config._deadtime/xsc),
         _config._leading_edge ? 'T':'F');
  for(unsigned i=xlo, j=0; j<nx; i++,j++)
    printf("%f%c", entry.content(i), (j%10)==9 ? '\n':' ');
#endif

  ndarray<double,2> edges = psalg::find_edges(make_ndarray(entry.content()+xlo,nx),
                                              _config._baseline_value*sc,
                                              _config._threshold_value*sc,
                                              _config._fraction,
                                              _config._deadtime/xsc,
                                              _config._leading_edge);
  const unsigned n = edges.shape()[0];
  if (n) {
    output.resize(n);
    for(unsigned i=0; i<n; i++) {
      double edge[Edges::NumberOf];
      edge[0] = edges(i,0)/sc;
      edge[1] = (edges(i,1)+xlo)*xsc + d.xlow();
      output.append(edge);
    }
  }

  _entry->valid(e.time());
  return *_entry;
}

void EdgeFinder::_invalid() { _entry->invalid(); }

void EdgeFinderConfig::load(const char*& p)
{
  _xlo = DBL_MIN;
  _xhi = DBL_MAX;
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
    else if (tag.name == "_xlo") 
      _xlo = QtPersistent::extract_d(p);
    else if (tag.name == "_xhi") 
      _xhi = QtPersistent::extract_d(p);
  XML_iterate_close(EdgeFinderConfig,tag);
}

void EdgeFinderConfig::save(char*& p) const
{
  XML_insert(p, "double", "_fraction"    , QtPersistent::insert(p,_fraction));
  XML_insert(p, "bool"  , "_leading_edge", QtPersistent::insert(p,_leading_edge));
  XML_insert(p, "double", "_deadtime"    , QtPersistent::insert(p,_deadtime));
  XML_insert(p, "double", "_threshold_value", QtPersistent::insert(p,_threshold_value));
  XML_insert(p, "double", "_baseline_value" , QtPersistent::insert(p,_baseline_value));
  XML_insert(p, "double", "_xlo" , QtPersistent::insert(p,_xlo));
  XML_insert(p, "double", "_xhi" , QtPersistent::insert(p,_xhi));
}

