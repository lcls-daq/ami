#include "ami/qt/ScalarPlotDesc.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescTH2F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"
//#include "ami/qt/DescText.hh"
#include "ami/qt/QtPersistent.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescTH2F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/DescScalarRange.hh"
#include "ami/data/DescScalarDRange.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>

using namespace Ami::Qt;

ScalarPlotDesc::ScalarPlotDesc(QWidget* parent, FeatureRegistry* registry, bool lNormWeight) :
  QWidget(parent)
{
  _title = new QLineEdit  ("name");
  _postB = new QPushButton("Post");
  _postB->setEnabled(false);

  _hist   = new DescTH1F  ("1dH");
  _vTime  = new DescChart ("v Time");
  _vFeature = new DescProf("Mean v Var" , registry);
  _vScan    = new DescScan("Mean v Scan", registry);
  _hist2d = new DescTH2F  ("2dH", registry);

  _xnorm = new QCheckBox("X");
  _ynorm = new QCheckBox("Y");
  _vnorm = new FeatureList(registry);
  _vnorm->use_scan(false);

  _weightB = new QCheckBox;
  _vweight = new FeatureList(registry);
  _vweight->use_scan(false);

  QVBoxLayout* layout1 = new QVBoxLayout;
  { QHBoxLayout* layout2 = new QHBoxLayout;
    layout2->addWidget(new QLabel("Entry name   Post:"));
    layout2->addWidget(_title);
    layout2->addStretch();
    layout2->addWidget(_postB);
    layout1->addLayout(layout2); }
  { QGroupBox* box = new QGroupBox("Plot Type");
    QVBoxLayout* vl = new QVBoxLayout;
    QTabWidget* tab = new QTabWidget;
    tab->addTab(_hist    , _hist    ->button()->text());
    tab->addTab(_vTime   , _vTime   ->button()->text());
    tab->addTab(_vFeature, _vFeature->button()->text());
    tab->addTab(_vScan   , _vScan   ->button()->text());
    tab->addTab(_hist2d  , _hist2d  ->button()->text());
    //    tab->addTab(_text    , _text    ->button()->text());
    vl->addWidget(tab);
    _plot_grp = tab;
    box->setLayout(vl);
    layout1->addWidget(box); }
  { QGroupBox* box = new QGroupBox;
    QVBoxLayout* ll = new QVBoxLayout;
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addStretch();
      hl->addWidget(new QLabel("Normalize"));
      { QVBoxLayout* vl = new QVBoxLayout;
	vl->addWidget(_xnorm);
	vl->addWidget(_ynorm);
	hl->addLayout(vl); }
      hl->addWidget(new QLabel("variable to"));
      hl->addWidget(_vnorm);
      ll->addLayout(hl); }
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addStretch();
      hl->addWidget(_weightB);
      hl->addWidget(new QLabel("Weight by"));
      hl->addWidget(_vweight); 
      hl->addStretch();
      ll->addLayout(hl); }
    box->setLayout(ll); 
    if (!lNormWeight) box->hide();
    layout1->addWidget(box); }
  setLayout(layout1);
}

ScalarPlotDesc::~ScalarPlotDesc()
{
}

void ScalarPlotDesc::save(char*& p) const
{
  XML_insert(p, "DescTH1F" , "_hist"    , _hist    ->save(p) );
  XML_insert(p, "DescChart", "_vTime"   , _vTime   ->save(p) );
  XML_insert(p, "DescProf" , "_vFeature", _vFeature->save(p) );
  XML_insert(p, "DescScan" , "_vScan"   , _vScan   ->save(p) );
  XML_insert(p, "DescTH2F" , "_hist2d"  , _hist2d  ->save(p) );
  XML_insert(p, "QButtonGroup", "_plot_grp", QtPersistent::insert(p,_plot_grp->currentIndex ()) );
}

void ScalarPlotDesc::load(const char*& p)
{
  XML_iterate_open(p,tag)
   if (tag.name == "_hist")
      _hist    ->load(p);
    else if (tag.name == "_vTime")
      _vTime   ->load(p);
    else if (tag.name == "_vFeature")
      _vFeature->load(p);
    else if (tag.name == "_vScan")
      _vScan   ->load(p);
    else if (tag.name == "_hist2d")
      _hist2d  ->load(p);
    else if (tag.name == "_plot_grp")
      _plot_grp->setCurrentIndex(QtPersistent::extract_i(p));
  XML_iterate_close(ScalarPlotDesc,tag);
}

Ami::DescEntry* ScalarPlotDesc::desc(const char* title) const
{
  DescEntry* desc = 0;

  QString vn = QString("(%1)/(%2)").arg(title).arg(_vnorm->entry());

  QString qtitle = title ? QString(title) : _title->text();

  switch(_plot_grp->currentIndex()) {
  case ScalarPlotDesc::TH1F:
    { QString v = _ynorm->isChecked() ? vn : qtitle;
      switch(_hist->method()) {
      case DescBinning::Fixed:
        desc = new Ami::DescTH1F(qPrintable(v),qPrintable(v),"events",
                                 _hist->bins(),_hist->lo(),_hist->hi()); 
        break;
      case DescBinning::Auto1:
        desc = new Ami::DescScalarRange(qPrintable(v),"events",
                                        DescScalarRange::MeanSigma,
                                        _hist->sigma(),
                                        _hist->nsamples(),
                                        _hist->bins());
        break;
      case DescBinning::Auto2:
        desc = new Ami::DescScalarRange(qPrintable(v),"events",
                                        DescScalarRange::MinMax,
                                        _hist->extent(),
                                        _hist->nsamples(),
                                        _hist->bins());
        break;
      default:
        break;
      }
      break; }
  case ScalarPlotDesc::vT: 
    { QString v = _ynorm->isChecked() ? vn : qtitle;
      switch(_vTime->stat()) {
      case Ami::DescScalar::StdDev:
        desc = new Ami::DescScalar(qPrintable(v),"stddev", Ami::DescScalar::StdDev,
                                   _weightB->isChecked() ? qPrintable(_vweight->entry()) : "",
                                   _vTime->pts(),
                                   _vTime->dpt());
        break;
      case Ami::DescScalar::Mean:
      default:
        desc = new Ami::DescScalar(qPrintable(v),"mean", Ami::DescScalar::Mean,
                                   _weightB->isChecked() ? qPrintable(_vweight->entry()) : "",
                                   _vTime->pts(),
                                   _vTime->dpt());
        break; }
      break; }
  case ScalarPlotDesc::vF:
    { QString vy = _ynorm->isChecked() ? vn : qtitle;
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vFeature->expr()).arg(_vnorm->entry()) : _vFeature->expr();
      desc = new Ami::DescProf(qPrintable(vy),
			       qPrintable(vx),"mean",
			       _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean",
			       _weightB->isChecked() ? qPrintable(_vweight->entry()) : "");
      break; }
  case ScalarPlotDesc::vS:
    { QString vy = _ynorm->isChecked() ? vn : qtitle;
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vScan->expr()).arg(_vnorm->entry()) : _vScan->expr();
      desc = new Ami::DescScan(qPrintable(vy),
			       qPrintable(_vScan->expr()),title,
			       _vScan->bins(),
			       _weightB->isChecked() ? qPrintable(_vweight->entry()) : "");
    }
    break;
  case ScalarPlotDesc::TH2F:
    { QString vy = _ynorm->isChecked() ? vn : qtitle;
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_hist2d->expr()).arg(_vnorm->entry()) : _hist2d->expr();
      double ex = _hist2d->xbins().method()==DescBinning::Auto1 ? _hist2d->xbins().sigma() : _hist2d->xbins().extent();
      double ey = _hist2d->ybins().method()==DescBinning::Auto1 ? _hist2d->ybins().sigma() : _hist2d->ybins().extent();
      if (_hist2d->xbins().method()==DescBinning::Fixed &&
          _hist2d->ybins().method()==DescBinning::Fixed)
        desc = new Ami::DescTH2F(qPrintable(vy),
                                 qPrintable(vx),qPrintable(vy),
                                 _hist2d->xbins().bins(),_hist2d->xbins().lo(),_hist2d->xbins().hi(),
                                 _hist2d->ybins().bins(),_hist2d->ybins().lo(),_hist2d->ybins().hi());
      else if (_hist2d->xbins().method()==DescBinning::Fixed) 
        desc = new Ami::DescScalarDRange(qPrintable(vy),
                                         qPrintable(vx),qPrintable(vy),
                                         _hist2d->ybins().nsamples(),
                                         _hist2d->xbins().bins(), _hist2d->xbins().lo(), _hist2d->xbins().hi(),
                                         _hist2d->ybins().method()==DescBinning::Auto2 ? DescScalarDRange::MinMax : DescScalarDRange::MeanSigma, ey,
                                         _hist2d->ybins().bins());
      else if (_hist2d->ybins().method()==DescBinning::Fixed) 
        desc = new Ami::DescScalarDRange(qPrintable(vy),
                                         qPrintable(vx),qPrintable(vy),
                                         _hist2d->xbins().nsamples(),
                                         _hist2d->xbins().method()==DescBinning::Auto2 ? DescScalarDRange::MinMax : DescScalarDRange::MeanSigma, ex,
                                         _hist2d->xbins().bins(),
                                         _hist2d->ybins().bins(), _hist2d->ybins().lo(), _hist2d->ybins().hi());
      else
        desc = new Ami::DescScalarDRange(qPrintable(vy),
                                         qPrintable(vx),qPrintable(vy),
                                         _hist2d->xbins().nsamples() > _hist2d->ybins().nsamples() ?
                                         _hist2d->xbins().nsamples() : _hist2d->ybins().nsamples(),
                                         _hist2d->xbins().method()==DescBinning::Auto2 ? DescScalarDRange::MinMax : DescScalarDRange::MeanSigma, ex,
                                         _hist2d->xbins().bins(),
                                         _hist2d->ybins().method()==DescBinning::Auto2 ? DescScalarDRange::MinMax : DescScalarDRange::MeanSigma, ey,
                                         _hist2d->ybins().bins());

      break; }
  default:
    desc = 0;
    break;
  }
  return desc;
}

const char* ScalarPlotDesc::expr(const QString& e) const 
{
  static char buf[256];
  QString vn = _ynorm->isChecked() ?
    QString("(%1)/(%2)").arg(e).arg(_vnorm->entry()) : e;
  strncpy(buf, qPrintable(vn), 256);
  return buf;
}

const char* ScalarPlotDesc::title() const
{
  return qPrintable(_title->text());
}

QString ScalarPlotDesc::qtitle() const
{ 
  return _title->text();
}

void ScalarPlotDesc::post(QObject* obj, const char* slot)
{
  connect(_postB, SIGNAL(clicked()), obj, slot);
  _postB->setEnabled(true);
}

bool ScalarPlotDesc::postAnalysis() const
{
  static QString _post_str("Post:");

  bool post(false);
  if (_ynorm->isChecked() && _vnorm->entry().contains(_post_str))
    post = true;

  switch(_plot_grp->currentIndex()) {

  case ScalarPlotDesc::vT:
    if (_weightB->isChecked() && _vweight->entry().contains(_post_str))
      post = true;
    break;

  case ScalarPlotDesc::vS:
    if (_vScan->expr().contains(_post_str))
      post = true;
    if (_xnorm->isChecked() && _vnorm->entry().contains(_post_str))
      post = true;
    if (_weightB->isChecked() && _vweight->entry().contains(_post_str))
      post = true;
    break;

  case ScalarPlotDesc::vF:
    if (_vFeature->expr().contains(_post_str))
      post = true;
    if (_xnorm->isChecked() && _vnorm->entry().contains(_post_str))
      post = true;
    if (_weightB->isChecked() && _vweight->entry().contains(_post_str))
      post = true;
    break;

  case ScalarPlotDesc::TH2F:
    if (_hist2d->expr().contains(_post_str))
      post = true;
    if (_xnorm->isChecked() && _vnorm->entry().contains(_post_str))
      post = true;
    break;

  default:
    break;
  }
  return post;
}
