#include "FIR.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/valgnd.hh"

#include "pdsalg/pdsalg.h"

#include <string.h>
#include <stdio.h>

#include <vector>

using namespace Ami;

FIR::FIR(const char* path) :
  AbsOperator(AbsOperator::FIR),
  _output    (0)
{
  strncpy_val(_path,path,PATH_LEN);
}

FIR::FIR(const char*& p, const DescEntry& e) :
  AbsOperator(AbsOperator::FIR)
{
  _extract(p,_path, PATH_LEN);

  std::vector<double> response;

  FILE* f = fopen(_path,"r");
  if (f) {
    float v;
    while(!feof(f)) {
      if (fscanf(f,"%f",&v)==1)
        response.push_back(v);
    }
  }

  if (response.size()==0)
    response.push_back(1.);

  _response = make_ndarray<double>(response.size());
  for(unsigned i=0; i<response.size(); i++)
    _response[i] = response[i];

  if (e.type() != DescEntry::Waveform) {
    printf("FIR constructed for type %s\n",DescEntry::type_str(e.type()));
    _output = 0;
  }
  else {
    DescWaveform desc(static_cast<const DescWaveform&>(e));
    unsigned ni = desc.nbins();
    unsigned no = _response.shape()[0]-1;
    desc.params(ni-no, desc.xlow(),
                (desc.xlow()*double(no)+desc.xup()*double(ni-no))/double(ni));
    _output  = EntryFactory::entry(desc);
    unsigned shape[] = {ni-no};
    _outputa = ndarray<double,1>(static_cast<EntryWaveform*>(_output)->content(),
                                 shape);
  }
}

FIR::~FIR()
{
  if (_output) delete _output;
}

DescEntry& FIR::_routput   () const { return _output->desc(); }

void*      FIR::_serialize(void* p) const
{
  _insert(p, _path , PATH_LEN);
  return p;
}

Entry&     FIR::_operate(const Entry& e) const
{
  if (!e.valid())
    return *_output;

  switch(e.desc().type()) {
  case DescEntry::Waveform:
    { EntryWaveform& entry = *static_cast<EntryWaveform*>(_output);
      if (e.valid()) {
        const EntryWaveform& input = static_cast<const EntryWaveform&>(e);
        unsigned shape[] = {input.desc().nbins()};
        ndarray<const double,1> in(input.content(),shape);
        pdsalg::finite_impulse_response(_response,
                                        in,
                                        const_cast<FIR*>(this)->_outputa);
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

void FIR::_invalid() { _output->invalid(); }
