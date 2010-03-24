#include "ami/qt/ScalarPlotDesc.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"
#include "ami/qt/QtPersistent.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QVBoxLayout>

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

  QVBoxLayout* layout1 = new QVBoxLayout;
  layout1->addWidget(_hist );
  layout1->addWidget(_vTime);
  layout1->addWidget(_vFeature);
  layout1->addWidget(_vScan);
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
  switch(_plot_grp->checkedId()) {
  case ScalarPlotDesc::TH1F:
    desc = new Ami::DescTH1F(title,
			     title,"events",
			     _hist->bins(),_hist->lo(),_hist->hi()); 
    break;
  case ScalarPlotDesc::vT: 
    desc = new Ami::DescScalar(title,"mean");
    break;
  case ScalarPlotDesc::vF:
    desc = new Ami::DescProf(title,
			     qPrintable(_vFeature->expr()),"mean",
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean");
    break;
  case ScalarPlotDesc::vS:
    desc = new Ami::DescScan(title,
			     qPrintable(_vScan->expr()),title,
			     _vScan->bins());
    break;
  default:
    desc = 0;
    break;
  }
  return desc;
}
