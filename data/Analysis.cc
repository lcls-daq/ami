#include "Analysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"

#include "ami/data/FilterFactory.hh"
#include "ami/data/OperatorFactory.hh"

//#define DBUG

using namespace Ami;

Analysis::Analysis(unsigned      id, 
		   const Entry&  input, 
		   unsigned      output, 
		   Cds&          cds, 
		   FeatureCache& icache,
		   FeatureCache& ocache,
		   const char*&  p) :
  _id    (id),
  _output(output),
  _input (input),
  _cds   (cds)
{
  FilterFactory filters(icache);
  _filter = filters.deserialize(p);

  OperatorFactory operators(icache,ocache);
  _op = operators.deserialize(p, input, cds, output);

  bool lvalid = input.valid();
  const_cast<Entry&>(input).invalid();
  Entry& output_entry = (*_op)(input);
  if (lvalid)
    const_cast<Entry&>(input).valid(input.time());

  if (_op->valid()) {
    _cds.add(&output_entry,output);
  }
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

void   Analysis::use()
{
  _input.desc().used(true);
  _op->use();
  _filter->use();
}

void   Analysis::analyze()
{
#ifdef DBUG
  printf("Analysis::analyze filter(%s)[%s] input(%s)[%s] op(%s)\n",
         _filter->text().c_str(), _filter->accept() ? "Acc":"Rej",
         _input.desc().name(), _input.valid()?"Valid":"Invalid",
         _op->type_str(_op->type()));
#endif

  if (_input.valid() && _filter->accept())
    (*_op)(_input);
  else
    _op->invalid(); 
}

const DescEntry& Analysis::output () const
{
  return _op->output();
}

bool Analysis::valid() const { return _op->valid(); }

const Entry& Analysis::input() const { return _input; }
