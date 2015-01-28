#include "ami/qt/EnvOp.hh"

#include "ami/data/AbsFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryCache.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryTH2F.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryProf2D.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryScalarRange.hh"
#include "ami/data/EntryScalarDRange.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/OperatorFactory.hh"

using Ami::XML::QtPersistent;
using namespace Ami::Qt;

EnvOp::EnvOp(const Ami::AbsFilter&  filter,
	     DescEntry*             desc,
	     Ami::ScalarSet         set) :
  _filter  (filter.clone()),
  _desc    (desc),
  _op      (0),
  _set     (set),
  _channel (0),
  _output_signature(0)
{
}

EnvOp::EnvOp(const Ami::AbsFilter&  filter,
	     DescEntry*             desc,
	     AbsOperator*           op,
	     unsigned               channel) :
  _filter  (filter.clone()),
  _desc    (desc),
  _op      (op),
  _set     (Ami::PreAnalysis),
  _channel (channel),
  _output_signature(0)
{
}

EnvOp::EnvOp() :
  _filter(0),
  _desc  (0),
  _op    (0),
  _set   (Ami::PreAnalysis),
  _channel(0),
  _output_signature(0)
{
}

EnvOp::~EnvOp()
{
  if (_filter) delete _filter;
  if (_desc)   delete _desc;
  if (_op)     delete _op;
}

void EnvOp::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "AbsFilter", "_filter", QtPersistent::insert(p, buff, (char*)_filter->serialize(buff)-buff) );
  if (_desc) {
    XML_insert( p, "DescEntry", "_desc", QtPersistent::insert(p, _desc, _desc->size()) );
  }
  if (_op) {
    XML_insert( p, "AbsOperator", "_op", QtPersistent::insert(p, buff, (char*)_op->serialize(buff)-buff) );
  }
  XML_insert( p, "ScalarSet", "_set" , QtPersistent::insert(p, int(_set)) );
  XML_insert( p, "unsigned" , "_channel" , QtPersistent::insert(p, _channel) );
  delete[] buff;
}

bool EnvOp::load(const Ami::XML::StartTag& tag,
		 const char*& p)
{
  bool result=true;
  if (tag.name == "_filter") {
    Ami::FilterFactory factory;
    const char* b = (const char*)QtPersistent::extract_op(p);
    _filter = factory.deserialize(b);
  }
  else if (tag.name == "_desc") {
    DescEntry* desc = (DescEntry*)QtPersistent::extract_op(p);

#define CASEENTRY(type) case DescEntry::type: _desc = new Desc##type(*static_cast<Desc##type*>(desc)); break;
   
    switch(desc->type()) {
      CASEENTRY(TH1F)
	CASEENTRY(Prof)
	CASEENTRY(Prof2D)
	CASEENTRY(Scan)
	CASEENTRY(Scalar)
	CASEENTRY(ScalarDRange)
	CASEENTRY(TH2F)
	CASEENTRY(Cache)
	default: break;
    }
  }
  else if (tag.name == "_op") {
    const char* b = (const char*)QtPersistent::extract_op(p);
    _op = Ami::OperatorFactory::deserialize(b);
  }
  else if (tag.name == "_set") {
    _set = Ami::ScalarSet(QtPersistent::extract_i(p));
  }
  else if (tag.name == "_channel") {
    _channel = QtPersistent::extract_i(p);
  }
  else
    result=false;
  return result;
}

void EnvOp::configure(char*& p, unsigned input, unsigned& output,
                      ConfigureRequest::Source source,
		      const AbsOperator& op)
{
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                  source,
						  input,
						  -1,
						  *_filter, op, _set);
  p += r.size();
  _req.request(r, output, _forceRequest());
  _output_signature = r.output();
}

void EnvOp::configure(char*& p, unsigned input, unsigned& output,
                      ConfigureRequest::Source source)
{
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                  source,
						  input,
						  -1,
						  *_filter, *_op, _set);
  p += r.size();
  _req.request(r, output, _forceRequest());
  _output_signature = r.output();
}







