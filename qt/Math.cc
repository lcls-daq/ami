#include "Math.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescChart.hh"

#include "ami/data/DescScalar.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Single.hh"
#include "ami/data/Average.hh"
#include "ami/data/Integral.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QFileDialog>
#include <QtGui/QComboBox>

#include <sys/socket.h>

// works for labels not buttons
//#define bold(t) "<b>" #t "</b>"
#define bold(t) #t

using namespace Ami::Qt;

enum { _Single, _Average, _IntegralTH1F, _IntegralvT, _IntegralvBLD, _IntegralvPV };

Math::Math(const char* name) :
  QWidget   (0)
{
  strcpy(_name,name);
  QLabel* lname = new QLabel(name);
  lname->setFrameShape(QFrame::Box);
  _ref = new QLabel;
  _ref->setFrameShape(QFrame::Box);
  QPushButton* loadB = new QPushButton("Load");
  _expr = new QLineEdit;
  _expr->setText("Ch");
  QPushButton* rstB = new QPushButton("Reset");
  //  new MathValidator(_expr);
  _sum_lo = new QLineEdit;
  _sum_hi = new QLineEdit;
  QPushButton* sumB = new QPushButton("Select Range");
  _plot_grp = new QButtonGroup;
  _sum_rng     = new DescTH1F(bold(Sum (1dH)));
  _sumvt_rng   = new DescChart(bold(Sum v Time));
  _sumvbld_rng = new DescProf(bold(Sum v BLD));
  _sumvpv_rng  = new DescProf(bold(Sum v PV));
  QPushButton* applyB = new QPushButton("Apply");
  _ref_entry = 0;
  _operator  = new Single;

  QVBoxLayout* l = new QVBoxLayout;
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QGridLayout* layout = new QGridLayout;
    QRadioButton* singleB  = new QRadioButton(bold(Single));
    QRadioButton* averageB = new QRadioButton(bold(Averaged));
    layout->addWidget(singleB     ,_Single      ,0); _plot_grp->addButton(singleB , _Single); 
    layout->addWidget(averageB    ,_Average     ,0); _plot_grp->addButton(averageB, _Average);
    layout->addWidget(_sum_rng    ,_IntegralTH1F,0); _plot_grp->addButton(_sum_rng    ->button(),_IntegralTH1F);
    layout->addWidget(_sumvt_rng  ,_IntegralvT  ,0); _plot_grp->addButton(_sumvt_rng  ->button(),_IntegralvT);
    layout->addWidget(_sumvbld_rng,_IntegralvBLD,0); _plot_grp->addButton(_sumvbld_rng->button(),_IntegralvBLD);
    layout->addWidget(_sumvpv_rng ,_IntegralvPV ,0); _plot_grp->addButton(_sumvpv_rng ->button(),_IntegralvPV);
    singleB->setChecked(true);
    plot_box->setLayout(layout);
    l->addWidget(plot_box); }
  { QGroupBox* def_box = new QGroupBox("Definitions");
    QVBoxLayout* layout = new QVBoxLayout;
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel(bold(Ch)));
      layout1->addWidget(lname);
      layout1->addStretch();
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel(bold(Ref)));
      layout1->addWidget(_ref);
      layout1->addWidget(loadB);
      layout1->addStretch();
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel(bold(Math)));
      layout1->addWidget(_expr);
      layout1->addWidget(rstB);
      layout1->addStretch();
      layout->addLayout(layout1); }
    { QHBoxLayout* layout1 = new QHBoxLayout;
      layout1->addWidget(new QLabel(bold(Sum)));
      layout1->addWidget(_sum_lo);
      layout1->addWidget(new QLabel("-"));
      layout1->addWidget(_sum_hi);
      layout1->addWidget(sumB);
      layout1->addStretch();
      layout->addLayout(layout1); }
    def_box->setLayout(layout);
    l->addWidget(def_box); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(applyB);
    layout->addStretch();
    l->addLayout(layout);
  }
  setLayout(l);

  connect(loadB , SIGNAL(clicked()), this, SLOT(load_ref()));
  connect(rstB  , SIGNAL(clicked()), this, SLOT(reset_math()));
  connect(sumB  , SIGNAL(clicked()), this, SLOT(sum_select()));
  connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));

  _applyB = applyB;
}

Math::~Math()
{
  delete _operator;
  if (_ref_entry) delete _ref_entry;
}

void Math::load_ref()
{
  //  Bring up file dialog and load reference
  //  Valid references must be of same type
  QString ref_dir("ref/");
  ref_dir += QString(_name);
  QString file = QFileDialog::getOpenFileName(this,"Reference File:",
					      ref_dir, "*.ami");
  if (file.isNull())
    return;

  FILE* f = fopen(qPrintable(file),"r");

  char* desc = new char[sizeof(DescEntry)];
  fread(desc,sizeof(DescEntry),1,f);
  const DescEntry& d = *reinterpret_cast<const DescEntry*>(desc);
  
  char* payload = new char[d.size()];
  fread(payload,d.size(),1,f);
  if (_ref_entry) delete _ref_entry;
  _ref_entry = EntryFactory::entry(d);
  iovec iov;
  iov.iov_base = payload;
  iov.iov_len  = d.size();
  _ref_entry->payload(iov);
}

void Math::reset_math()
{
  _expr->setText("Ch");
  _plot_grp->button(0)->setChecked(true);
  apply();
}

void Math::sum_select()
{
  // Grab bins from mouse click on "display"
  // ("display" function?)
  _sum_xlo = 0;
  _sum_xhi = 0;
  _sum_ylo= 0;
  _sum_yhi= 0;
}

void Math::apply()
{
  // Form _operator
  delete _operator;

  switch(_plot_grp->checkedId()) {
  case _Average     : 
    _operator = new Average; 
    break;
  case _IntegralTH1F: 
    _operator = new Integral(Ami::DescTH1F("sum","projection","events",
					   _sum_rng->bins(),_sum_rng->lo(),_sum_rng->hi()),
			     _sum_xlo, _sum_xhi,
			     _sum_ylo, _sum_yhi); 
    break;
  case _IntegralvT: 
    _operator = new Integral(Ami::DescScalar("sum","projection"),
			     _sum_xlo, _sum_xhi,
			     _sum_ylo, _sum_yhi); 
    break;
  case _IntegralvBLD:
  case _IntegralvPV:
    printf("Integral v BLD/PV not yet implemented.  Defaulting to Single.\n");
  case _Single:
  default:
    _operator = new Single ; break;
  }
}

Ami::AbsOperator* Math::math() const
{
  return _operator;
}

