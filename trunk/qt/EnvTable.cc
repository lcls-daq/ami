#include "ami/qt/EnvTable.hh"

#include "ami/data/EnvPlot.hh"
#include "ami/data/Cds.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/AbsFilter.hh"
#include "ami/data/FilterFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/QtPersistent.hh"

#include "ami/qt/QtTable.hh"
#include <QtCore/QString>

#include <stdio.h>

using namespace Ami::Qt;

EnvTable::EnvTable(QWidget*,
                   const Ami::AbsFilter& filter,
                   DescScalar*      desc,
                   Ami::ScalarSet   set) :
  EnvOp    (filter,desc,set),
  _plot    (new QtTable)
{
  connect(this, SIGNAL(update_plot()), _plot, SLOT(update()));
  connect(_plot, SIGNAL(remove()), this, SLOT(remove()));
}

EnvTable::EnvTable(QWidget*, const char*& p)
{
  XML_iterate_open(p,tag)
    if (EnvOp::load(tag,p))
      ;
  XML_iterate_close(EnvPost,tag);

  _plot = new QtTable;
  connect(this, SIGNAL(update_plot()), _plot, SLOT(update()));
  connect(_plot, SIGNAL(remove()), this, SLOT(remove()));
}

EnvTable::~EnvTable()
{
  delete _plot;
}

void EnvTable::load(const char*& p)
{
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
  emit update_plot();
}

void EnvTable::remove()
{
  emit remove(this);
}
