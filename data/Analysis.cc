#include "Analysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"

#include "ami/data/FilterFactory.hh"
#include "ami/data/OperatorFactory.hh"

using namespace Ami;

Analysis::Analysis(unsigned      id, 
		   const Entry&  input, 
		   unsigned      output, 
		   Cds&          cds, 
		   FeatureCache& cache,
		   const char*&  p) :
  _id    (id),
  _output(output),
  _input (input),
  _cds   (cds)
{
  FilterFactory filters(cache);
  _filter = filters.deserialize(p);

  OperatorFactory operators(cache);
  _op = operators.deserialize(p, input, cds, output);

  bool lvalid = input.valid();
  const_cast<Entry&>(input).invalid();
  Entry& output_entry = (*_op)(input);
  if (lvalid)
    const_cast<Entry&>(input).valid(input.time());

  if (_op->valid())
    _cds.add(&output_entry,output);
}

Analysis::~Analysis() 
{
  if (_op->valid())
    _cds.remove(_cds.entry(_output));
  delete _filter;
  // bug, possibly double free of entries
  // consequence of not deleting is a small mem-leak
  delete _op;
}

unsigned   Analysis::id() const { return _id; }

void   Analysis::analyze()
{
  if (_input.valid() && _filter->accept())
    (*_op)(_input);
}

DescEntry& Analysis::output () const
{
  return _op->output();
}

bool Analysis::valid() const { return _op->valid(); }
