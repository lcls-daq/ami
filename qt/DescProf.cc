#include "DescProf.hh"
#include "ami/qt/QtPersistent.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Calculator.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

using namespace Ami::Qt;


DescProf::DescProf(const char* name) :
  QWidget(0), _button(new QRadioButton(name)), 
  _bins(new QLineEdit("100")), _lo(new QLineEdit("0")), _hi(new QLineEdit("1")),
  _expr(new QLabel)
{
  QPushButton* calcB = new QPushButton("X Var");

  _bins->setMaximumWidth(60);
  _lo  ->setMaximumWidth(60);
  _hi  ->setMaximumWidth(60);
  new QIntValidator   (_bins);
  new QDoubleValidator(_lo);
  new QDoubleValidator(_hi);
  QVBoxLayout* layout1 = new QVBoxLayout;
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(_button);
    //    layout->addWidget(_features);
    layout->addWidget(calcB);
    layout->addStretch();
    layout1->addLayout(layout); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(new QLabel("bins"));
    layout->addWidget(_bins);
    layout->addWidget(new QLabel("lo"));
    layout->addWidget(_lo);
    layout->addWidget(new QLabel("hi"));
    layout->addWidget(_hi);
    layout1->addLayout(layout); }
  layout1->addWidget(_expr);
  setLayout(layout1);

  connect(calcB, SIGNAL(clicked()), this, SLOT(calc()));
}


static QChar _exponentiate(0x005E);
static QChar _multiply    (0x00D7);
static QChar _divide      (0x00F7);
static QChar _add         (0x002B);
static QChar _subtract    (0x002D);

void DescProf::calc()
{
  QStringList ops;
  ops << _exponentiate
      << _multiply
      << _divide
      << _add   
      << _subtract;

  QStringList vops;

  Calculator* c = new Calculator(tr("X Var Math"), "",
				 FeatureRegistry::instance().names(),
				 vops, ops);
  if (c->exec()==QDialog::Accepted)
    _expr->setText(c->result());

  delete c;
}

QRadioButton* DescProf::button() { return _button; }

QString  DescProf::expr() const { return _expr->text(); }

QString  DescProf::feature() const 
{
  QString e(_expr->text());
  QString new_expr;
  {
    int pos=0;
    const QStringList& slist = FeatureRegistry::instance().names();
    while(1) {
      int npos = e.size();
      const QString* spos = 0;
      for(QStringList::const_iterator it=slist.begin(); it!=slist.end(); it++) {
	int np = e.indexOf(*it, pos);
	if ((np >=0 && np < npos) || (np==npos && it->size()>spos->size())) {
	  npos = np;
	  spos = &*it;
	}
      }
      if (spos) {
	new_expr.append(e.mid(pos,npos-pos));
	new_expr.append(QString("[%1]").arg(FeatureRegistry::instance().index(*spos)));
	pos = npos + spos->size();
      }
      else 
	break;
      printf("new_expr %s\n",qPrintable(new_expr));
    }
    new_expr.append(e.mid(pos));
  }
  printf("new_expr %s\n",qPrintable(new_expr));
  return new_expr;
}

unsigned DescProf::bins() const { return _bins->text().toInt(); }
double   DescProf::lo  () const { return _lo->text().toDouble(); }
double   DescProf::hi  () const { return _hi->text().toDouble(); }

void DescProf::save(char*& p) const
{
  QtPersistent::insert(p,_expr ->text());
  QtPersistent::insert(p,_bins->text());
  QtPersistent::insert(p,_lo  ->text());
  QtPersistent::insert(p,_hi  ->text());
}

void DescProf::load(const char*& p)
{
  _expr->setText(QtPersistent::extract_s(p));
  _bins->setText(QtPersistent::extract_s(p));
  _lo  ->setText(QtPersistent::extract_s(p));
  _hi  ->setText(QtPersistent::extract_s(p));
}

