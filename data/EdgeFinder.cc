#include "EdgeFinder.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include <stdio.h>

using namespace Ami;

EdgeFinder::EdgeFinder(unsigned   signature,
		       double     fraction,
		       double     threshold_value,
		       double     baseline_value,
		       const      DescTH1F& output) :
  AbsOperator(AbsOperator::EdgeFinder),
  _input(signature),
  _fraction (fraction),
  _threshold_value(threshold_value),
  _baseline_value(baseline_value),
  _output(output),
  _input_entry(0),
  _output_entry(0)
{
}

EdgeFinder::EdgeFinder(unsigned input, const EdgeFinder& f) :
  AbsOperator(AbsOperator::EdgeFinder),
  _input          (input),
  _fraction       (f._fraction),
  _threshold_value(f._threshold_value),
  _baseline_value (f._baseline_value),
  _output         (f._output),
  _input_entry    (0),
  _output_entry   (0)
{
}

#define CASETERM(type) case DescEntry::type: \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

static const char* _advance(const char*& p, unsigned size) { const char* o=p; p+=size; return o; }
#define EXTRACT(p, type) *(reinterpret_cast<const type*>(_advance(p,sizeof(type))))

EdgeFinder::EdgeFinder(const char*& p, const Cds& cds) :
  AbsOperator     (AbsOperator::EdgeFinder),
  _input          (EXTRACT(p, unsigned)),
  _threshold_value(EXTRACT(p, double)),
  _baseline_value (EXTRACT(p, double)),
  _output         (EXTRACT(p, DescTH1F)),
  _output_entry   (new EntryTH1F(_output))
{
  _input_entry = static_cast<const EntryWaveform*>(cds.entry(_input));
  printf("EF input %p\n",_input_entry);
}

EdgeFinder::~EdgeFinder()
{
  if (_input_entry)
    delete _input_entry;
  if (_output_entry)
    delete _output_entry;
}

unsigned   EdgeFinder::input    () const { return _input; }
DescEntry& EdgeFinder::output   () const { return _output_entry->desc(); }

void*      EdgeFinder::_serialize(void* p) const
{
  _insert(p, &_input, sizeof(_input));
  _insert(p, &_threshold_value, sizeof(_threshold_value));
  _insert(p, &_baseline_value, sizeof(_baseline_value));
  _insert(p, &_output, sizeof(_output));
  return p;
}

Entry&     EdgeFinder::_operate(const Entry& e) const
{
  const EntryWaveform& entry = *_input_entry;
  const DescWaveform& d = entry.desc();
  // find the boundaries where the pulse crosses the threshold
  double   peak;
  unsigned start  =0;
  bool     crossed=false;
  bool     rising = _threshold_value > _baseline_value;
  for(unsigned k=0; k<d.nbins(); k++) {
    double y = entry.content(k);
    bool over = 
      ( rising && y>_threshold_value) ||
      (!rising && y<_threshold_value);
    if (!crossed && over) {
      crossed = true;
      start   = k;
      peak    = y;
    }
    else if (crossed && !over) {
      //  find the edge
      double edge_v = 0.5*(peak+_baseline_value);
      unsigned i=start;
      if (rising) { // leading edge +
	while(entry.content(i) < edge_v)
	  i++;
      }
      else {        // leading edge -
	while(entry.content(i) > edge_v)
	  i++;
      }
      double edge = i>0 ? 
	(edge_v-entry.content(i))/(entry.content(i)-entry.content(i-1)) 
	+ double(i) : 0;
      _output_entry->addcontent(1.,edge*(d.xup()-d.xlow())/double(d.nbins())+d.xlow());
      crossed = false;
    }
    else if (( rising && y>peak) ||
	     (!rising && y<peak))
      peak = y;
  }
  return *_output_entry;
}
