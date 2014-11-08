#include "ami/qt/LineFitPlotDesc.hh"

#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescProf2D.hh"
#include "ami/qt/DescScan.hh"
#include "ami/data/QtPersistent.hh"

#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescProf2D.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/LineFit.hh"

#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QRadioButton>

#include <stdio.h>

using namespace Ami::Qt;

static inline int avgRound(int n, int d)
{
  return (n+d-1)/d;
}

enum { Slope, Intercept };

LineFitPlotDesc::LineFitPlotDesc(QWidget* parent, FeatureRegistry* registry) :
  QWidget(parent),
  _method(new QComboBox),
  _stat  (new QComboBox)
{
  for(unsigned i=0; i<Ami::LineFit::NumberOf; i++)
    _method->addItem(Ami::LineFit::method_str(Ami::LineFit::Method(i)));
  _method->setCurrentIndex(0);

  _stat->addItem("Slope");
  _stat->addItem("Intercept");
  _stat->setCurrentIndex(0);

  _vTime  = new DescChart ("v Time",false);
  _vFeature  = new DescProf("Mean v Var" , registry);
  _vFeature2 = new DescProf2D("Mean v Var2D" , registry);
  _vScan    = new DescScan("Mean v Scan", registry);

  QVBoxLayout* layout1 = new QVBoxLayout;
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(new QLabel("Method"));
    hl->addWidget(_method);
    layout1->addLayout(hl); }
  { QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(new QLabel("Plot"));
    hl->addWidget(_stat);
    layout1->addLayout(hl); }
  { QGroupBox* box = new QGroupBox("Plot Type");
    QVBoxLayout* vl = new QVBoxLayout;
    QTabWidget* tab = new QTabWidget;
    tab->addTab(_vTime    , _vTime    ->button()->text());
    tab->addTab(_vFeature , _vFeature ->button()->text());
    tab->addTab(_vScan    , _vScan    ->button()->text());
    tab->addTab(_vFeature2, _vFeature2->button()->text());
    vl->addWidget(tab);
    _plot_grp = tab;
    box->setLayout(vl);
    layout1->addWidget(box); }
  setLayout(layout1);
}

LineFitPlotDesc::~LineFitPlotDesc()
{
}

void LineFitPlotDesc::save(char*& p) const
{
  XML_insert(p, "QComboBox", "_method", QtPersistent::insert(p,_method->currentIndex ()) );
  XML_insert(p, "QComboBox", "_stat", QtPersistent::insert(p,_stat->currentIndex ()) );
  XML_insert(p, "DescChart" , "_vTime"    , _vTime    ->save(p) );
  XML_insert(p, "DescProf"  , "_vFeature" , _vFeature ->save(p) );
  XML_insert(p, "DescProf2D", "_vFeature2", _vFeature2->save(p) );
  XML_insert(p, "DescScan"  , "_vScan"    , _vScan    ->save(p) );
  XML_insert(p, "QTabWidget", "_plot_grp", QtPersistent::insert(p,_plot_grp->currentIndex ()) );
}

void LineFitPlotDesc::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_method")
      _method  ->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_stat")
      _stat    ->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_vTime")
      _vTime   ->load(p);
    else if (tag.name == "_vFeature")
      _vFeature->load(p);
    else if (tag.name == "_vFeature2")
      _vFeature2->load(p);
    else if (tag.name == "_vScan")
      _vScan   ->load(p);
    else if (tag.name == "_plot_grp")
      _plot_grp->setCurrentIndex(QtPersistent::extract_i(p));
  XML_iterate_close(LineFitPlotDesc,tag);
}

Ami::DescEntry* LineFitPlotDesc::desc(const char* title) const
{
  DescEntry* desc = 0;

  Ami::DescEntry::Stat stat;
  std::string          stat_name;
  switch(_stat->currentIndex()) {
  case Slope    : stat=Ami::DescEntry::Slope; stat_name="slope"; break;
  case Intercept: stat=Ami::DescEntry::Intercept; stat_name="intercept"; break;
  default: break;
  }

  const char* method_str = Ami::LineFit::method_str(Ami::LineFit::Method(_method->currentIndex()));

  switch(_plot_grp->currentIndex()) {
  case LineFitPlotDesc::vT: 
    desc = new Ami::DescScalar(title, stat_name.c_str(), stat,
			       method_str, _vTime->pts(), _vTime->dpt());
    break;
  case LineFitPlotDesc::vF:
    desc = new Ami::DescProf(title,
			     qPrintable(_vFeature->expr()), stat_name.c_str(),
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),
			     "", stat, method_str);
    break;
  case LineFitPlotDesc::vF2:
    desc = new Ami::DescProf2D(title,
			       qPrintable(_vFeature2->xexpr()),
			       qPrintable(_vFeature2->yexpr()),
			       _vFeature2->xbins(),_vFeature2->xlo(),_vFeature2->xhi(),
			       _vFeature2->ybins(),_vFeature2->ylo(),_vFeature2->yhi(),
			       stat_name.c_str(), stat, method_str);
    break;
  case LineFitPlotDesc::vS:
    desc = new Ami::DescScan(title,
			     qPrintable(_vScan->expr()),stat_name.c_str(),
			     _vScan->bins(),
			     method_str, stat);
    break;
  default:
    desc = 0;
    break;
  }
  return desc;
}

Ami::DescEntry* LineFitPlotDesc::table(const char* title) const
{
  DescEntry* desc = 0;
  const char* method_str = Ami::LineFit::method_str(Ami::LineFit::Method(_method->currentIndex()));

  switch(_stat->currentIndex()) {
  case Slope:
    desc = new Ami::DescScalar(title, "slope", Ami::DescScalar::Slope,
			       method_str,1,1);
    break;
  case Intercept:
  default:
    desc = new Ami::DescScalar(title,"intercept", Ami::DescScalar::Intercept,
			       method_str,1,1);
    break;
  }
  return desc;
}

QString LineFitPlotDesc::stat() const
{
  return _stat->currentText();
}

bool LineFitPlotDesc::postAnalysis() const
{
  static QString _post_str("Post:");

  bool post(false);

  switch(_plot_grp->currentIndex()) {

  case LineFitPlotDesc::vS:
    if (_vScan->expr().contains(_post_str))
      post = true;
    break;

  case LineFitPlotDesc::vF:
    if (_vFeature->expr().contains(_post_str))
      post = true;
    break;

  case LineFitPlotDesc::vF2:
    if (_vFeature2->xexpr().contains(_post_str))
      post = true;
    if (_vFeature2->yexpr().contains(_post_str))
      post = true;
    break;

  default:
    break;
  }
  return post;
}

