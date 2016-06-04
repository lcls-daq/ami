#include "DescTH1F.hh"
#include "ami/data/QtPersistent.hh"

#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QVBoxLayout>

using namespace Ami::Qt;

DescTH1F::DescTH1F(const char* name, bool autoRange, bool normalize, bool aggregate) :
  DescBinning("bins", autoRange),
  _button(new QRadioButton(name)),
  _normalize(normalize ? new QCheckBox("Normalize area to unity") : 0),
  _aggregate_grp(0)
{
  if (normalize) {
    QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
    l->addWidget(_normalize);
    _normalize->setChecked(true);
  }
  if (aggregate) {
    _aggregate_grp = new QButtonGroup;
    QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
    QHBoxLayout* l1 = new QHBoxLayout;
    QRadioButton* b0 = new QRadioButton("Single Event");
    QRadioButton* b1 = new QRadioButton("Sum of Processes");
    b1->setChecked(true);
    l1->addWidget(b0);
    l1->addWidget(b1);
    _aggregate_grp->addButton(b0);
    _aggregate_grp->addButton(b1);
    l->addLayout(l1);
  }
}

DescTH1F::~DescTH1F()
{
  if (_aggregate_grp) delete _aggregate_grp;
}

QRadioButton* DescTH1F::button() { return _button; }

bool DescTH1F::normalize() const { return _normalize ? _normalize->isChecked() : false; }

bool DescTH1F::aggregate() const { return _aggregate_grp ? _aggregate_grp->checkedId()==1 : false; }

void DescTH1F::save(char*& p) const
{
  XML_insert(p, "DescBinning", "self", DescBinning::save(p));
  XML_insert(p, "QCheckBox", "_normalize", QtPersistent::insert(p,normalize()));
}

void DescTH1F::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.element == "DescBinning")
      DescBinning::load(p);
    else if (tag.name == "_normalize") {
      if (_normalize) 
	_normalize->setChecked(QtPersistent::extract_b(p));
    }
  XML_iterate_close(DescTH1F,tag);
}
