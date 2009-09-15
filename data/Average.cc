#include "Average.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include <stdio.h>

using namespace Ami;

Average::Average(unsigned n) : 
  AbsOperator(AbsOperator::Average),
  _n         (n),
  _entry     (0),
  _cache     (0)
{
}

Average::Average(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::Average)
{
  _extract(p, &_n, sizeof(_n));
  _entry = EntryFactory::entry(e);
  _cache = _n ? EntryFactory::entry(e) : (Entry*)0;
}

Average::~Average()
{
  if (_entry) delete _entry;
  if (_cache) delete _cache;
}

DescEntry& Average::output   () const { return _n ? _cache->desc() : _entry->desc(); }

void*      Average::_serialize(void* p) const
{
  _insert(p, &_n, sizeof(_n));
  return p;
}

Entry&     Average::_operate(const Entry& e) const
{
  switch(e.desc().type()) {
  case DescEntry::TH1F:
    printf("Averaging TH1F not implemented\n");
    break;
  case DescEntry::TH2F:
    printf("Averaging TH2F not implemented\n");
    break;
  case DescEntry::Prof:
    printf("Averaging Prof not implemented\n");
    break;
  case DescEntry::Image:
    { const EntryImage& en = static_cast<const EntryImage&>(e);
      EntryImage& _en = static_cast<EntryImage&>(*_entry);
      for(unsigned j=0; j<en.desc().nbinsy(); j++)
	for(unsigned k=0; k<en.desc().nbinsx(); k++)
	  _en.addcontent(en.content(k,j),k,j);
      for(unsigned j=0; j<EntryImage::InfoSize; j++) {
	EntryImage::Info i = (EntryImage::Info)j;
	_en.addinfo(en.info(i),i);
      }
      if (_en.info(EntryImage::Normalization)==_n) {
	static_cast<EntryImage*>(_cache)->setto(_en);
	_en.reset();
      }
      break; }
  case DescEntry::Waveform:
    { const EntryWaveform& en = static_cast<const EntryWaveform&>(e);
      EntryWaveform& _en = static_cast<EntryWaveform&>(*_entry);
      for(unsigned k=0; k<en.desc().nbins(); k++)
	_en.addcontent(en.content(k),k);
      for(unsigned j=0; j<EntryWaveform::InfoSize; j++) {
	EntryWaveform::Info i = (EntryWaveform::Info)j;
	_en.addinfo(en.info(i),i);
      }
      if (_n && _en.info(EntryWaveform::Normalization)==_n) {
	static_cast<EntryWaveform*>(_cache)->setto(_en);
	_en.reset();
      }
      break; }
  default:
    break;
  }
  return _n ? *_cache : *_entry;
}
