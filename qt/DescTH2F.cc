#include "DescTH2F.hh"
#include "ami/qt/QtPersistent.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/FeatureCalculator.hh"

#include <QtGui/QRadioButton>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QButtonGroup>

using namespace Ami::Qt;

#define USE_TH2F

DescTH2F::DescTH2F(const char* name, FeatureRegistry* registry) :
  QWidget(0), _button(new QRadioButton(name)),
  _xbins    (new DescBinning("xbins")),
  _ybins    (new DescBinning("ybins")),
  _expr (new QLineEdit),
  _registry(registry)
{
  QPushButton* calcB = new QPushButton("X Var");

#ifndef USE_TH2F
  QRadioButton* td_button = new QRadioButton("TH2F");
  QRadioButton* im_button = new QRadioButton("Image");

  _group = new QButtonGroup;
  _group->addButton(td_button,TH2F);
  _group->addButton(im_button,Image);
  td_button->setChecked(true);

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(_expr);
    hl->addWidget(calcB);
    { QVBoxLayout* vl = new QVBoxLayout;
      vl->addWidget(td_button);
      vl->addWidget(im_button);
      hl->addLayout(vl); }
    layout->addLayout(hl); }
  layout->addWidget(_xbins);
  layout->addWidget(_ybins);
  setLayout(layout);
#else
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addStretch();
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(_expr);
    hl->addWidget(calcB);
    layout->addLayout(hl); }
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(_xbins);
    hl->addStretch();
    hl->addWidget(_ybins);
    layout->addLayout(hl); }
  layout->addStretch();
  setLayout(layout);
#endif

  connect(calcB, SIGNAL(clicked()), this, SLOT(calc()));
}

#ifndef USE_TH2F
DescTH2F::Output DescTH2F::output() const { return Output(_group->checkedId()); }
#else
DescTH2F::Output DescTH2F::output() const { return TH2F; }
#endif

const DescBinning& DescTH2F::xbins() const { return *_xbins; }
const DescBinning& DescTH2F::ybins() const { return *_ybins; }

void DescTH2F::save(char*& p) const
{
  XML_insert( p, "DescBinning", "_xbins", _xbins->save(p) );
  XML_insert( p, "DescBinning", "_ybins", _ybins->save(p) );
  XML_insert( p, "QString", "_expr", QtPersistent::insert(p, _expr->text()) );
#ifndef USE_TH2F
  XML_insert( p, "QButtonGroup", "_group", QtPersistent::insert(p, _group->checkedId()) );
#endif
}

void DescTH2F::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_xbins")
      _xbins->load(p);
    else if (tag.name == "_ybins")
      _ybins->load(p);
    else if (tag.name == "_expr")
      _expr->setText(QtPersistent::extract_s(p));
#ifndef USE_TH2F
    else if (tag.name == "_group")
      _group->button(QtPersistent::extract_i(p))->setChecked(true);
#endif
  XML_iterate_close(DescTH2F,tag);
}

QString  DescTH2F::expr() const { return _expr->text(); }

void DescTH2F::calc()
{
  FeatureCalculator* c = new FeatureCalculator(this,tr("X Var Math"),*_registry);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

  delete c;
}

QRadioButton* DescTH2F::button() { return _button; }
