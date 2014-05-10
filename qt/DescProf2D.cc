#include "DescProf2D.hh"
#include "DescProf.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/FeatureCalculator.hh"
#include "ami/data/QtPersistent.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include <stdio.h>

using namespace Ami::Qt;


DescProf2D::DescProf2D(const char* name, FeatureRegistry* registry) :
  QWidget(0), _button(new QRadioButton(name)), 
  _xbins(new QLineEdit("100")), _xlo(new QLineEdit("0")), _xhi(new QLineEdit("1")),
  _xexpr(new QLineEdit),
  _ybins(new QLineEdit("100")), _ylo(new QLineEdit("0")), _yhi(new QLineEdit("1")),
  _yexpr(new QLineEdit),
  _registry(registry)
{
  QPushButton* xcalcB = new QPushButton("X Var");
  QPushButton* ycalcB = new QPushButton("Y Var");

  _xbins->setMaximumWidth(40); new QIntValidator   (_xbins);
  _xlo  ->setMaximumWidth(40); new QDoubleValidator(_xlo);
  _xhi  ->setMaximumWidth(40); new QDoubleValidator(_xhi);

  _ybins->setMaximumWidth(40); new QIntValidator   (_ybins);
  _ylo  ->setMaximumWidth(40); new QDoubleValidator(_ylo);
  _yhi  ->setMaximumWidth(40); new QDoubleValidator(_yhi);
  
  QGridLayout* layout = new QGridLayout;

  layout->addWidget(_xexpr,0,0,1,4);
  layout->addWidget(xcalcB,1,0,1,4);
  layout->addWidget(new QLabel("xbins"),2,1,1,1,::Qt::AlignRight);
  layout->addWidget(_xbins,2,2,1,1,::Qt::AlignLeft);
  layout->addWidget(new QLabel("lo"),3,0,1,1,::Qt::AlignRight);
  layout->addWidget(_xlo,3,1,1,1);
  layout->addWidget(new QLabel("hi"),3,2,1,1,::Qt::AlignRight);
  layout->addWidget(_xhi,3,3,1,1);
  
  layout->addWidget(_yexpr,0,4,1,4);
  layout->addWidget(ycalcB,1,4,1,4);
  layout->addWidget(new QLabel("ybins"),2,5,1,1,::Qt::AlignRight);
  layout->addWidget(_ybins,2,6,1,1,::Qt::AlignLeft);
  layout->addWidget(new QLabel("lo"),3,4,1,1,::Qt::AlignRight);
  layout->addWidget(_ylo,3,5,1,1);
  layout->addWidget(new QLabel("hi"),3,6,1,1,::Qt::AlignRight);
  layout->addWidget(_yhi,3,7,1,1);

  setLayout(layout);

  connect(xcalcB, SIGNAL(clicked()), this, SLOT(xcalc()));
  connect(ycalcB, SIGNAL(clicked()), this, SLOT(ycalc()));
}


void DescProf2D::xcalc()
{
  FeatureCalculator* c = new FeatureCalculator(this,tr("X Var Math"),*_registry);
  if (c->exec()==QDialog::Accepted)
    _xexpr->setText(c->result());

  delete c;
}

void DescProf2D::ycalc()
{
  FeatureCalculator* c = new FeatureCalculator(this,tr("Y Var Math"),*_registry);
  if (c->exec()==QDialog::Accepted)
    _yexpr->setText(c->result());

  delete c;
}

QRadioButton* DescProf2D::button() { return _button; }

QString  DescProf2D::xexpr() const { return _xexpr->text(); }
QString  DescProf2D::yexpr() const { return _yexpr->text(); }

unsigned DescProf2D::xbins() const { return _xbins->text().toInt(); }
double   DescProf2D::xlo  () const { return _xlo->text().toDouble(); }
double   DescProf2D::xhi  () const { return _xhi->text().toDouble(); }

unsigned DescProf2D::ybins() const { return _ybins->text().toInt(); }
double   DescProf2D::ylo  () const { return _ylo->text().toDouble(); }
double   DescProf2D::yhi  () const { return _yhi->text().toDouble(); }

void DescProf2D::save(char*& p) const
{
  XML_insert( p, "QString", "_xexpr",
              QtPersistent::insert(p,_xexpr ->text()) );
  XML_insert( p, "QString", "_xbins",
              QtPersistent::insert(p,_xbins->text()) );
  XML_insert( p, "QString", "_xlo",
              QtPersistent::insert(p,_xlo  ->text()) );
  XML_insert( p, "QString", "_xhi",
              QtPersistent::insert(p,_xhi  ->text()) );

  XML_insert( p, "QString", "_yexpr",
              QtPersistent::insert(p,_yexpr ->text()) );
  XML_insert( p, "QString", "_ybins",
              QtPersistent::insert(p,_ybins->text()) );
  XML_insert( p, "QString", "_ylo",
              QtPersistent::insert(p,_ylo  ->text()) );
  XML_insert( p, "QString", "_yhi",
              QtPersistent::insert(p,_yhi  ->text()) );
}

void DescProf2D::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if (tag.element == "QString") {
      if      (tag.name == "_xexpr")
        _xexpr->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_xbins")
        _xbins->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_xlo")
        _xlo  ->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_xhi")
        _xhi  ->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_yexpr")
        _yexpr->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_ybins")
        _ybins->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_ylo")
        _ylo  ->setText(QtPersistent::extract_s(p));
      else if (tag.name == "_yhi")
        _yhi  ->setText(QtPersistent::extract_s(p));
    }
  XML_iterate_close(DescProf2D,tag);
}

