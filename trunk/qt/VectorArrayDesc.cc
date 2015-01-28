#include "ami/qt/VectorArrayDesc.hh"

#include "ami/qt/AmendedRegistry.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/XML.hh"

#include <QtCore/QStringList>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

using Ami::XML::QtPersistent;
using namespace Ami::Qt;

VectorArrayDesc::VectorArrayDesc(QWidget* p,
			   const QStringList& parms) : 
  QWidget(p) 
{
  _parameter = new QComboBox;
  _parameter->addItems(parms);

  _registry = new AmendedRegistry(FeatureRegistry::instance(),parms);
  _desc = new ScalarPlotDesc(0,_registry);

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* l = new QHBoxLayout;
    l->addStretch();
    l->addWidget(new QLabel("Parameter"));
    l->addWidget(_parameter);
    l->addStretch();
    layout->addLayout(l); }
  layout->addWidget(_desc);
  
  setLayout(layout);
} 
VectorArrayDesc::~VectorArrayDesc()
{
  delete _registry; 
}

QString VectorArrayDesc::title() const
{ return _parameter->currentText(); }

const char* VectorArrayDesc::expression() const 
{ return _desc->expr(QString("[%1]").arg(_parameter->currentIndex())); }

bool        VectorArrayDesc::postAnalysis() const 
{ return _desc->postAnalysis(); }

Ami::DescEntry* VectorArrayDesc::desc(const char* title) const {
  Ami::DescEntry* d = _desc->desc(title);
  _registry->translate(const_cast<char*>(d->xtitle()));
  _registry->translate(const_cast<char*>(d->ytitle()));
  return d;
}

void VectorArrayDesc::save(char*& p) const
{
  XML_insert(p, "QComboBox", "_parameter", QtPersistent::insert(p, _parameter->currentIndex()));
  XML_insert(p, "ScalarPlotDesc", "_desc", _desc->save(p));
}

void VectorArrayDesc::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_parameter")
      _parameter->setCurrentIndex( QtPersistent::extract_i(p) );
    else if (tag.name == "_desc")
      _desc->load(p);
  XML_iterate_close(Droplet,tag);
}
