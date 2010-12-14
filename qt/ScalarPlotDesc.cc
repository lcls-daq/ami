#include "ami/qt/ScalarPlotDesc.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"
#include "ami/qt/FeatureBox.hh"
#include "ami/qt/QtPersistent.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>

using namespace Ami::Qt;

ScalarPlotDesc::ScalarPlotDesc(QWidget* parent) :
  QWidget(parent)
{
  _hist   = new DescTH1F  ("Sum (1dH)");
  _vTime  = new DescChart ("Mean v Time",0.2);
  _vFeature = new DescProf("Mean v Var" );
  _vScan    = new DescScan("Mean v Scan");

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_hist    ->button(),(int)TH1F);
  _plot_grp->addButton(_vTime   ->button(),(int)vT);
  _plot_grp->addButton(_vFeature->button(),(int)vF);
  _plot_grp->addButton(_vScan   ->button(),(int)vS);
  _hist->button()->setChecked(true);

  _xnorm = new QCheckBox("X");
  _ynorm = new QCheckBox("Y");
  _vnorm = new FeatureBox;

  _weightB = new QCheckBox;
  _vweight = new FeatureBox;

  QVBoxLayout* layout1 = new QVBoxLayout;
  { QGroupBox* box = new QGroupBox("Plot Type");
    QVBoxLayout* vl = new QVBoxLayout;
    vl->addWidget(_hist );
    vl->addWidget(_vTime);
    vl->addWidget(_vFeature);
    vl->addWidget(_vScan);
    box->setLayout(vl);
    layout1->addWidget(box); }
  { QGroupBox* box = new QGroupBox("Normalization");
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addStretch();
    hl->addWidget(new QLabel("Normalize"));
    { QVBoxLayout* vl = new QVBoxLayout;
      vl->addWidget(_xnorm);
      vl->addWidget(_ynorm);
      hl->addLayout(vl); }
    hl->addWidget(new QLabel("variable to"));
    hl->addWidget(_vnorm);
    box->setLayout(hl);
    layout1->addWidget(box); }
  { QGroupBox* box = new QGroupBox("Weighted Average");
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addStretch();
    hl->addWidget(_weightB);
    hl->addWidget(new QLabel("Weight by"));
    hl->addWidget(_vweight); 
    hl->addStretch();
    box->setLayout(hl); 
    layout1->addWidget(box); }
  setLayout(layout1);
}

ScalarPlotDesc::~ScalarPlotDesc()
{
}

void ScalarPlotDesc::save(char*& p) const
{
  _hist    ->save(p);
  _vTime   ->save(p);
  _vFeature->save(p);

  QtPersistent::insert(p,_plot_grp->checkedId ());
}

void ScalarPlotDesc::load(const char*& p)
{
  _hist    ->load(p);
  _vTime   ->load(p);
  _vFeature->load(p);

  _plot_grp->button(QtPersistent::extract_i(p))->setChecked(true);
}

Ami::DescEntry* ScalarPlotDesc::desc(const char* title) const
{
  DescEntry* desc = 0;

  QString vn = QString("(%1)/(%2)").arg(title).arg(_vnorm->entry());

  switch(_plot_grp->checkedId()) {
  case ScalarPlotDesc::TH1F:
    { QString v = _xnorm->isChecked() ? vn : QString(title);
      desc = new Ami::DescTH1F(qPrintable(v),qPrintable(v),"events",
			     _hist->bins(),_hist->lo(),_hist->hi()); 
      break; }
  case ScalarPlotDesc::vT: 
    { QString v = _xnorm->isChecked() ? vn : QString(title);
      desc = new Ami::DescScalar(qPrintable(v),"mean",
				 _weightB->isChecked() ? qPrintable(_vweight->entry()) : "");
      break; }
  case ScalarPlotDesc::vF:
    { QString vy = _ynorm->isChecked() ? vn : QString(title);
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vFeature->expr()).arg(_vnorm->entry()) : _vFeature->expr();
      desc = new Ami::DescProf(qPrintable(vy),
			       qPrintable(vx),"mean",
			       _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean",
			       _weightB->isChecked() ? qPrintable(_vweight->entry()) : "");
      break; }
  case ScalarPlotDesc::vS:
    { QString vy = _ynorm->isChecked() ? vn : QString(title);
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vScan->expr()).arg(_vnorm->entry()) : _vScan->expr();
      desc = new Ami::DescScan(qPrintable(vy),
			       qPrintable(_vScan->expr()),title,
			       _vScan->bins(),
			       _weightB->isChecked() ? qPrintable(_vweight->entry()) : "");
      break; }
  default:
    desc = 0;
    break;
  }
  return desc;
}

const char* ScalarPlotDesc::expr(const QString& e) const 
{
  QString vn = _ynorm->isChecked() ?
    QString("(%1)/(%2)").arg(e).arg(_vnorm->entry()) : e;
  return qPrintable(vn);
}
