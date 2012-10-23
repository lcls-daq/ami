#include "ami/qt/EnvTable.hh"

#include "ami/data/EnvPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/qt/QtPersistent.hh"

#include "ami/qt/QtTable.hh"
#include <QtCore/QString>


using namespace Ami::Qt;

EnvTable::EnvTable(QWidget*,
                   const Ami::AbsFilter& filter,
                   DescScalar*      desc,
                   Ami::ScalarSet   set) :
  _filter  (filter.clone()),
  _desc    (desc),
  _set     (set),
  _output_signature(0),
  _plot    (new QtTable(this))
{
}

EnvTable::EnvTable(QWidget*, const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.name == "_filter") {
      Ami::FilterFactory factory;
      const char* b = (const char*)QtPersistent::extract_op(p);
      _filter = factory.deserialize(b);
    }
    else if (tag.name == "_desc") {
      _desc = new DescScalar(*(DescScalar*)QtPersistent::extract_op(p));
    }
    else if (tag.name == "_set") {
      _set = Ami::ScalarSet(QtPersistent::extract_i(p));
    }
  XML_iterate_close(EnvPost,tag);

  _plot = new QtTable(this);
}

EnvTable::~EnvTable()
{
  delete _desc;
}

void EnvTable::save(char*& p) const
{
  char* buff = new char[8*1024];
  XML_insert( p, "AbsFilter", "_filter", QtPersistent::insert(p, buff, (char*)_filter->serialize(buff)-buff) );
  XML_insert( p, "DescScalar", "_desc", QtPersistent::insert(p, _desc, _desc->size()) );
  XML_insert( p, "ScalarSet", "_set" , QtPersistent::insert(p, int(_set)) );
  delete[] buff;
}

void EnvTable::load(const char*& p)
{
}

void EnvTable::configure(char*& p, unsigned input, unsigned& output)
{
  Ami::EnvPlot op(*_desc);

  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
						  ConfigureRequest::Discovery,
						  input,
						  -1,
						  *_filter, op, _set);
  p += r.size();
  _req.request(r,output);
  _output_signature = r.output();
}

void EnvTable::setup_payload(Cds& cds)
{
  Ami::Entry* entry = cds.entry(_output_signature);
  if (entry)
    _plot->entry(static_cast<Ami::EntryScalar*>(entry));
  else
    _plot->entry(0);
}

void EnvTable::update()
{
  _plot->update();
}

void EnvTable::remove()
{
  delete this;
}
