#include "VAPlot.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/VectorArray.hh"

#include "ami/data/Cds.hh"

#include <stdio.h>

//#define DBUG

using namespace Ami;

VAPlot::VAPlot(int ix,
	       int iy,
	       int iz,
	       int    accumulate,
	       const DescImage& o) :
  AbsOperator(AbsOperator::VAPlot),
  _ix          (ix),
  _iy          (iy),
  _iz          (iz),
  _accumulate  (accumulate),
  _current     (0),
  _output_entry(0),
  _cache       (0)
{
  memcpy(_desc, &o, o.size());

#ifdef DBUG
  printf("VAPlot:ctor ix,iy,iz [%d,%d,%d]  acc %d  o.nx,ny [%d,%d]\n",
	 ix,iy,iz,accumulate,o.nbinsx(),o.nbinsy());
#endif
}

#define CASETERM(type) case DescEntry::type: \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

static const char* _advance(const char*& p, unsigned size) { const char* o=p; p+=size; return o; }
#define EXTRACT(p, type) *(reinterpret_cast<const type*>(_advance(p,sizeof(type))))

VAPlot::VAPlot(const char*& p) :
  AbsOperator  (AbsOperator::VAPlot),
  _ix          (EXTRACT(p, int   )),
  _iy          (EXTRACT(p, int   )),
  _iz          (EXTRACT(p, int   )),
  _accumulate  (EXTRACT(p, int   )),
  _current     (0),
  _output_entry(0),
  _cache       (0)
{
  memcpy(_desc, p, sizeof(DescImage));
}

VAPlot::VAPlot(const char*& p, const DescEntry& e) :
  AbsOperator  (AbsOperator::PeakFinder),
  _ix          (EXTRACT(p, int   )),
  _iy          (EXTRACT(p, int   )),
  _iz          (EXTRACT(p, int   )),
  _accumulate  (EXTRACT(p, int   )),
  _current     (0),
  _cache       (0)
{
  DescImage::deserialize(_desc,p);
  const DescImage& o = *reinterpret_cast<const DescImage*>(_desc);
  _output_entry = static_cast<EntryImage*>(EntryFactory::entry(o));

  _output_entry->info(0,EntryImage::Pedestal);
  _output_entry->desc().aggregate(_accumulate>=0);
  _output_entry->desc().normalize(_accumulate<0);
  _output_entry->desc().countmode(_iz<0);

  if (_accumulate > 0) {
    _cache = static_cast<EntryImage*>(EntryFactory::entry(_output_entry->desc()));
    _cache->info(0,EntryImage::Pedestal);
  }

#ifdef DBUG
  printf("VAPlot:ctor[2] ix,iy,iz [%d,%d,%d]  acc %d  o.nx,ny [%d,%d]\n",
	 _ix,_iy,_iz,_accumulate,o.nbinsx(),o.nbinsy());
#endif
}

VAPlot::~VAPlot()
{
  if (_output_entry)
    delete _output_entry;
  if (_cache)
    delete _cache;
}

DescEntry& VAPlot::_routput   () const { return _accumulate<=0 ? _output_entry->desc() : _cache->desc(); }

void*      VAPlot::_serialize(void* p) const
{
  _insert(p, &_ix          , sizeof(_ix));
  _insert(p, &_iy          , sizeof(_iy));
  _insert(p, &_iz          , sizeof(_iz));
  _insert(p, &_accumulate  , sizeof(_accumulate));
  _insert(p, &_desc        , sizeof(DescImage));
  return p;
}

Entry&     VAPlot::_operate(const Entry& e) const
{
  if (!e.valid()) 
    return *_output_entry;

  if (e.desc().type() != DescEntry::Ref) {
    printf("VAPlot::_operator on %d type %s \n",
	   e.desc().signature(),DescEntry::type_str(e.desc().type()));
    abort();
  }

  const EntryRef& entry = static_cast<const EntryRef&>(e);
  const VectorArray& a  = *reinterpret_cast<const VectorArray*>(entry.data());

  if (_accumulate<0)
    _output_entry->reset();

  if (_iz<0)
    for(unsigned i=0; i<a.nentries(); i++)
      _output_entry->addcontent(1, 
				unsigned(a.element(_ix)[i]),
				unsigned(a.element(_iy)[i]));
  else
    for(unsigned i=0; i<a.nentries(); i++)
      _output_entry->addcontent(unsigned(a.element(_iz)[i]),
				unsigned(a.element(_ix)[i]),
				unsigned(a.element(_iy)[i]));

  Entry* output;
  _output_entry->valid(e.time());
  if (_accumulate >= 0) {
    if (_iz>=0)
      _output_entry->addinfo(1, EntryImage::Normalization);
    if (_accumulate == 0) {
      output = _output_entry;
    }
    else {
      if (++_current >= _accumulate) {
	_cache->setto(*_output_entry);
	_output_entry->reset();
	_current = 0;
      }
      else
	_cache->invalid();
      output = _cache;
    }
  }
  else {
    _output_entry->info(1, EntryImage::Normalization);
    output =_output_entry;
  }
  return *output;
}

void VAPlot::_invalid() { _output_entry->invalid(); }
