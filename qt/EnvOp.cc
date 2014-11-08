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

using Ami::XML::QtPersistent;
using namespace Ami::Qt;

EnvOp::EnvOp(const Ami::AbsFilter&  filter,
	     DescEntry*             desc,
	     Ami::ScalarSet         set) :
  _filter  (filter.clone()),
  _desc    (desc),
  _set     (set),
  _output_signature(0)
{
}

EnvOp::EnvOp() :
  _filter(0),
  _desc  (0),
  _set   (Ami::PreAnalysis),
  _output_signature(0)
{
}

EnvOp::~EnvOp()
{
  if (_filter) delete _filter;
  delete _desc;
}

void EnvOp::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "AbsFilter", "_filter", QtPersistent::insert(p, buff, (char*)_filter->serialize(buff)-buff) );
  XML_insert( p, "DescEntry", "_desc", QtPersistent::insert(p, _desc, _desc->size()) );
  XML_insert( p, "ScalarSet", "_set" , QtPersistent::insert(p, int(_set)) );
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
  else if (tag.name == "_set") {
    _set = Ami::ScalarSet(QtPersistent::extract_i(p));
  }
  else
    result=false;
  return result;
}

void EnvOp::configure(char*& p, unsigned input, unsigned& output,
		      const AbsOperator& op, bool forceRequest)
{
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Discovery,
						  input,
						  -1,
						  *_filter, op, _set);
  p += r.size();
  _req.request(r, output, forceRequest);
  _output_signature = r.output();
}



