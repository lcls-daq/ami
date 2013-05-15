#include "FIR.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include <string.h>
#include <stdio.h>

using namespace Ami;

FIR::FIR(const char* path) :
  AbsOperator(AbsOperator::FIR),
  _output    (0),
  _response  (0)
{
  if (path)
    strncpy(_path,path,PATH_LEN);
  else
    memset(_path,0,PATH_LEN);
}

FIR::FIR(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::FIR)
{
  _extract(p,_path, PATH_LEN);

  _response = new std::vector<float>;

  FILE* f = fopen(_path,"r");
  if (f) {
    float v;
    while(!feof(f)) {
      if (fscanf(f,"%f",&v)==1)
        _response->push_back(v);
    }
  }

  if (_response->size()==0)
    _response->push_back(1.);

  if (e.type() != DescEntry::Waveform) {
    printf("FIR constructed for type %s\n",DescEntry::type_str(e.type()));
    _output = 0;
  }
  else {
    DescWaveform desc(static_cast<const DescWaveform&>(e));
    unsigned ni = desc.nbins();
    unsigned no = _response->size()-1;
    desc.params(ni-no, desc.xlow(),
                (desc.xlow()*double(no)+desc.xup()*double(ni-no))/double(ni));
    _output = EntryFactory::entry(desc);
    _output->reset();
  }
}

FIR::~FIR()
{
  if (_output) delete _output;
  if (_response) delete _response;
}

DescEntry& FIR::_routput   () const { return _output->desc(); }

void*      FIR::_serialize(void* p) const
{
  _insert(p, _path , PATH_LEN);
  return p;
}

Entry&     FIR::_operate(const Entry& e) const
{
  switch(e.desc().type()) {
  case DescEntry::Waveform:
    { EntryWaveform& entry = *static_cast<EntryWaveform*>(_output);
      if (e.valid()) {
        const EntryWaveform& input = static_cast<const EntryWaveform&>(e);
        for(unsigned i=0; i<entry.desc().nbins(); i++) {
          double v=0;
          for(unsigned j=0; j<_response->size(); j++)
            v += (*_response)[j]*input.content(i+j);
          entry.content(v,i);
        }
        for(unsigned j=0; j<EntryWaveform::InfoSize; j++) {
          EntryWaveform::Info i = (EntryWaveform::Info)j;
          entry.info(input.info(i),i);
        }
      }
      break;
    }
  default:
    printf("FIR operating on type %s\n",DescEntry::type_str(e.desc().type()));
    break;
  }

  _output->valid(e.time());
  return *_output;
}

