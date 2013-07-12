#include "PeakFinder.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/PeakFinderFn.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include <stdio.h>

using namespace Ami;

static PeakFinderFn* _lookup(unsigned);

PeakFinder::PeakFinder(double threshold_v0,
                       double threshold_v1,
                       Mode   mode,
                       bool   center_only,
                       int    accumulate) :
  AbsOperator(AbsOperator::PeakFinder),
  _threshold_v0(threshold_v0),
  _threshold_v1(threshold_v1),
  _mode        (mode),
  _center_only (center_only),
  _accumulate  (accumulate),
  _current     (0),
  _output_entry(0),
  _cache       (0),
  _fn          (0)
{
}

#define CASETERM(type) case DescEntry::type: \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

static const char* _advance(const char*& p, unsigned size) { const char* o=p; p+=size; return o; }
#define EXTRACT(p, type) *(reinterpret_cast<const type*>(_advance(p,sizeof(type))))

PeakFinder::PeakFinder(const char*& p) :
  AbsOperator     (AbsOperator::PeakFinder),
  _threshold_v0(EXTRACT(p, double)),
  _threshold_v1(EXTRACT(p, double)),
  _mode        (EXTRACT(p, Mode  )),
  _center_only (EXTRACT(p, bool  )),
  _accumulate  (EXTRACT(p, int   )),
  _current     (0),
  _output_entry(0),
  _cache       (0),
  _fn          (0)
{
}

PeakFinder::PeakFinder(const char*& p, const DescEntry& e) :
  AbsOperator     (AbsOperator::PeakFinder),
  _threshold_v0(EXTRACT(p, double)),
  _threshold_v1(EXTRACT(p, double)),
  _mode        (EXTRACT(p, Mode  )),
  _center_only (EXTRACT(p, bool  )),
  _accumulate  (EXTRACT(p, int   )),
  _current     (0),
  _output_entry(static_cast<EntryImage*>(EntryFactory::entry(e))),
  _cache       (0),
  _fn          (0)
{
  _output_entry->info(0,EntryImage::Pedestal);
  _output_entry->desc().aggregate(_accumulate>=0);
  _output_entry->desc().normalize(_accumulate<0);
  _output_entry->desc().countmode(_mode==Count);

  if (_accumulate > 0) {
    _cache = static_cast<EntryImage*>(EntryFactory::entry(_output_entry->desc()));
    _cache->info(0,EntryImage::Pedestal);
  }

  if ((_fn = _lookup(e.info().phy())))
    _fn->setup(_threshold_v0,
               _threshold_v1);
}

PeakFinder::~PeakFinder()
{
  if (_output_entry)
    delete _output_entry;
  if (_cache)
    delete _cache;
  if (_fn)
    delete _fn;
}

DescEntry& PeakFinder::_routput   () const { return _accumulate<=0 ? _output_entry->desc() : _cache->desc(); }

void*      PeakFinder::_serialize(void* p) const
{
  _insert(p, &_threshold_v0, sizeof(_threshold_v0));
  _insert(p, &_threshold_v1, sizeof(_threshold_v1));
  _insert(p, &_mode        , sizeof(_mode));
  _insert(p, &_center_only , sizeof(_center_only));
  _insert(p, &_accumulate  , sizeof(_accumulate));
  return p;
}

Entry&     PeakFinder::_operate(const Entry& e) const
{
  if (!e.valid()) 
    return *_output_entry;

  const EntryImage& entry = static_cast<const EntryImage&>(e);
  const DescImage& d = entry.desc();
  const unsigned nx = d.nbinsx();
  const unsigned ny = d.nbinsy();
  const unsigned p  = unsigned(entry.info(EntryImage::Pedestal));
  const unsigned dn = d.ppxbin()*d.ppybin();
  const unsigned q  = 1 + p;

  if (_accumulate<0) {
    _output_entry->reset();
    printf("PeakFinder)_operate() line 120 RESET !!!!\n");
  }

  // find the peak positions which are above the threshold
  const unsigned* a = entry.contents();
  if (_center_only) {
    for(unsigned j=0; j<nx; j++) {
      _output_entry->addcontent(p,j,0   );
      _output_entry->addcontent(p,j,ny-1);
    }
    for(unsigned k=1; k<ny-1; k++) {
      _output_entry->addcontent(p,0   ,k);
      _output_entry->addcontent(p,nx-1,k);
    }
    const unsigned* a = entry.contents();
    for(unsigned k=1; k<ny-1; k++, a+=nx) {
      const unsigned* b = a + nx;
      const unsigned* c = b + nx;
      for(unsigned j=1; j<nx-1; j++) {
        const unsigned threshold = p + dn*(_fn ? _fn->value(j,k): unsigned(_threshold_v0));
        unsigned v = b[j];
        if (!(v > threshold &&
              v > b[j-1] && 
              v > b[j+1] &&
              v > a[j-1] && 
              v > a[j+0] &&
              v > a[j+1] &&
              v > c[j-1] && 
              v > c[j+0] &&
              v > c[j+1]))     v = p;
        else if (_mode==Count) v = q;
        _output_entry->addcontent(v,j,k);
      }
    }
  }
  else {
    for(unsigned k=0; k<ny; k++) {
      for(unsigned j=0; j<nx; j++) {
        const unsigned threshold = p + dn*(_fn ? _fn->value(j,k) : unsigned(_threshold_v0));
        unsigned v = *a++;
        if      (v < threshold)  v = p;
        else if (_mode == Count) v = q;
        _output_entry->addcontent(v,j,k);
      }
    }
  }

  Entry* output;
  _output_entry->valid(e.time());
  if (_accumulate >= 0) {
    _output_entry->addinfo(p, EntryImage::Pedestal);
    if (_mode == Sum)
      _output_entry->addinfo(entry.info(EntryImage::Normalization), EntryImage::Normalization);
    if (_accumulate == 0) {
      output = _output_entry;
    }
    else {
      if (++_current >= _accumulate) {
	_cache->setto(*_output_entry);
	_output_entry->reset();
	printf("PeakFinder)_operate() line 179 RESET !!!!\n");
	_current = 0;
      }
      else
	_cache->invalid();
      output = _cache;
    }
  }
  else {
    _output_entry->info(p, EntryImage::Pedestal);
    _output_entry->info(1, EntryImage::Normalization);
    output =_output_entry;
  }
  return *output;
}

#include <map>

static std::map<unsigned,PeakFinderFn*> _fn_map;

void PeakFinder::register_(unsigned      phy,
                           PeakFinderFn* fn)
{
  if (_fn_map[phy])
    delete _fn_map[phy];

  _fn_map[phy] = fn;
}

PeakFinderFn* _lookup(unsigned phy)
{
  std::map<unsigned,PeakFinderFn*>::iterator it = _fn_map.find(phy);
  return (it == _fn_map.end()) ? 0 : _fn_map[phy]->clone();
}
