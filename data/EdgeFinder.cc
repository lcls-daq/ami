#include "EdgeFinder.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include "pdsalg/pdsalg.h"

#include <stdio.h>

using namespace Ami;

EdgeFinder::EdgeFinder(double     fraction,
		       double     threshold_value,
		       double     baseline_value,
                       int        alg,
                       double     deadtime,
		       const      DescEntry& output,
                       Parameter  parameter) :
  AbsOperator(AbsOperator::EdgeFinder),
  _fraction (fraction),
  _alg(alg),
  _deadtime(deadtime),
  _threshold_value(threshold_value),
  _baseline_value(baseline_value),
  _output_entry(0),
  _parameter(parameter)
{
  char* b = new char[output.size()];
  memcpy(b,&output,output.size());
  _output = reinterpret_cast<DescEntry*>(b);
}

#define CASETERM(type) case DescEntry::type: \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

static const char* _advance(const char*& p, unsigned size) { const char* o=p; p+=size; return o; }
#define EXTRACT(p, type) *(reinterpret_cast<const type*>(_advance(p,sizeof(type))))

EdgeFinder::EdgeFinder(const char*& p) :
  AbsOperator     (AbsOperator::EdgeFinder),
  _fraction       (EXTRACT(p, double)),
  _alg            (EXTRACT(p, int)),
  _deadtime       (EXTRACT(p, double)),
  _threshold_value(EXTRACT(p, double)),
  _baseline_value (EXTRACT(p, double))
{
  const DescEntry& output = *reinterpret_cast<const DescEntry*>(p);
  char* b = new char[output.size()];
  memcpy(b,p,output.size());
  p += output.size();
  _output = reinterpret_cast<DescEntry*>(b);

  _output_entry   = EntryFactory::entry(*_output);

  _parameter      = Parameter(EXTRACT(p, int));
}

EdgeFinder::EdgeFinder(double fraction, int alg, double deadtime, const char*& p) :
  AbsOperator     (AbsOperator::EdgeFinder),
  _fraction       (fraction),
  _alg            (alg),
  _deadtime       (deadtime),
  _threshold_value(EXTRACT(p, double)),
  _baseline_value (EXTRACT(p, double))
{
  const DescEntry& output = *reinterpret_cast<const DescEntry*>(p);
  char* b = new char[output.size()];
  memcpy(b,p,output.size());
  p += output.size();
  _output = reinterpret_cast<DescEntry*>(b);

  _output_entry   = EntryFactory::entry(*_output);

  _parameter      = Parameter(EXTRACT(p, int));
}

EdgeFinder::~EdgeFinder()
{
  delete _output;
  if (_output_entry)
    delete _output_entry;
}

//DescEntry& EdgeFinder::_routput   () const { return _output_entry->desc(); }
DescEntry& EdgeFinder::_routput   () const { return *_output; }
void* EdgeFinder::desc   () const { return (void *)_output; }
int EdgeFinder::desc_size   () const { return _output->size(); }

void*      EdgeFinder::_serialize(void* p) const
{
  _insert(p, &_fraction, sizeof(_fraction));
  _insert(p, &_alg, sizeof(_alg));
  _insert(p, &_deadtime, sizeof(_deadtime));
  _insert(p, &_threshold_value, sizeof(_threshold_value));
  _insert(p, &_baseline_value, sizeof(_baseline_value));
  _insert(p,  _output, _output->size());
  _insert(p, &_parameter, sizeof(_parameter));
  return p;
}

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

