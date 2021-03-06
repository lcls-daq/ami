#include "ami/qt/ScalarPlotDesc.hh"

#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescTH2F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescProf2D.hh"
#include "ami/qt/DescScan.hh"
//#include "ami/qt/DescText.hh"
#include "ami/qt/SMPRegistry.hh"
#include "ami/data/QtPersistent.hh"

#include "ami/data/DescTH1F.hh"
#include "ami/data/DescTH2F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescProf2D.hh"
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

#include <stdio.h>

//#define ACCUM_STATS

using namespace Ami::Qt;

static inline int avgRound(int n, int d)
{
  return (n+d-1)/d;
}

enum { Average, Sum, RMS, NAggs };

ScalarPlotDesc::ScalarPlotDesc(QWidget* parent, FeatureRegistry* registry, bool lNormWeight) :
  QWidget(parent)
{
  _title = new QLineEdit  ("name");
  _postB = new QPushButton("Post");
  _postB->setEnabled(false);

  _hist   = new DescTH1F  ("1dH");
  _vTime  = new DescChart ("v Time");
  _vFeature  = new DescProf("Mean v Var" , registry);
  _vFeature2 = new DescProf2D("Mean v Var2D" , registry);
  _vScan    = new DescScan("Mean v Scan", registry);
  _hist2d = new DescTH2F  ("2dH", registry);

#ifdef ACCUM_STATS
  _agg_grp = new QButtonGroup;
  QCheckBox* avgB = new QCheckBox("Avg");
  QCheckBox* sumB = new QCheckBox("Sum");
  QCheckBox* rmsB = new QCheckBox("RMS");
  _agg_grp->addButton(avgB,Average);
  _agg_grp->addButton(sumB,Sum);
  _agg_grp->addButton(rmsB,RMS);
  _agg_grp->setExclusive(false);
  connect(_agg_grp, SIGNAL(buttonClicked(int)), this, SLOT(aggClicked(int)));

  _interval  = new QLineEdit;
  _interval->setMaximumWidth(40);
  _intervalq = new QLabel;
  new QIntValidator(_interval);
  connect(_interval, SIGNAL(editingFinished()), this, SLOT(update_interval()));
#endif

  _xnorm = new QCheckBox("X");
  _ynorm = new QCheckBox("Y");
  _vnorm = new FeatureList(registry);
  _vnorm->use_scan(false);

  _weightB = new QCheckBox;
  _vweight = new FeatureList(registry);
  _vweight->use_scan(false);
  { QChar sigma(0x03A3);
    QString tip = QString("Averaged entries are weighted by w:\n"	\
			  " <avg> = (%1 w x)/(%2 w)\n"			\
			  "For ratios (n/d) consider w=d:\n"		\
			  " <avg> = (%3 n)/(%4 d)\n")
      .arg(sigma).arg(sigma)
      .arg(sigma).arg(sigma);
    _weightB->setToolTip(tip);
    _vweight->setToolTip(tip); }

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
    tab->addTab(_hist     , _hist     ->button()->text());
    tab->addTab(_vTime    , _vTime    ->button()->text());
    tab->addTab(_vFeature , _vFeature ->button()->text());
    tab->addTab(_vScan    , _vScan    ->button()->text());
    tab->addTab(_hist2d   , _hist2d   ->button()->text());
    tab->addTab(_vFeature2, _vFeature2->button()->text());
    //    tab->addTab(_text    , _text    ->button()->text());
    vl->addWidget(tab);
    _plot_grp = tab;
    box->setLayout(vl);
    layout1->addWidget(box); }
  { QHBoxLayout* hbox = new QHBoxLayout;
#ifdef ACCUM_STATS
    //
    //  Accumulate statistics before plotting
    //
    { QGroupBox* box = new QGroupBox;
      QGridLayout* gl = new QGridLayout;
      gl->addWidget(avgB,0,0,1,1);
      gl->addWidget(sumB,1,0,1,1);
      gl->addWidget(rmsB,2,0,1,1);
      gl->addWidget(_interval ,0,1,3,1);
      gl->addWidget(_intervalq,0,2,3,1);
      box->setLayout(gl);
      hbox->addWidget(box); }
#endif
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
      hbox->addWidget(box); }
    layout1->addLayout(hbox); }
  setLayout(layout1);
}

ScalarPlotDesc::~ScalarPlotDesc()
{
}

void ScalarPlotDesc::save(char*& p) const
{
  XML_insert(p, "DescTH1F"  , "_hist"     , _hist     ->save(p) );
  XML_insert(p, "DescChart" , "_vTime"    , _vTime    ->save(p) );
  XML_insert(p, "DescProf"  , "_vFeature" , _vFeature ->save(p) );
  XML_insert(p, "DescProf2D", "_vFeature2", _vFeature2->save(p) );
  XML_insert(p, "DescScan"  , "_vScan"    , _vScan    ->save(p) );
  XML_insert(p, "DescTH2F"  , "_hist2d"   , _hist2d   ->save(p) );
  XML_insert(p, "QButtonGroup", "_plot_grp", QtPersistent::insert(p,_plot_grp->currentIndex ()) );
#ifdef ACCUM_STATS
  XML_insert(p, "QButtonGroup", "_agg_grp" , QtPersistent::insert(p,_agg_grp->checkedId ()) );
  XML_insert(p, "QLineEdit"   , "_interval" , QtPersistent::insert(p,_interval->text()) );
#endif
}

void ScalarPlotDesc::load(const char*& p)
{
  int id = -1;

  XML_iterate_open(p,tag)
   if (tag.name == "_hist")
      _hist    ->load(p);
    else if (tag.name == "_vTime")
      _vTime   ->load(p);
    else if (tag.name == "_vFeature")
      _vFeature->load(p);
    else if (tag.name == "_vFeature2")
      _vFeature2->load(p);
    else if (tag.name == "_vScan")
      _vScan   ->load(p);
    else if (tag.name == "_hist2d")
      _hist2d  ->load(p);
    else if (tag.name == "_plot_grp")
      _plot_grp->setCurrentIndex(QtPersistent::extract_i(p));
    else if (tag.name == "_agg_grp")
      id = QtPersistent::extract_i(p);
#ifdef ACCUM_STATS
    else if (tag.name == "_interval")
      _interval->setText(QtPersistent::extract_s(p));
#endif
  XML_iterate_close(ScalarPlotDesc,tag);

#ifdef ACCUM_STATS
  for(int i=0; i<NAggs; i++)
    _agg_grp->button(i)->setChecked(i==id);
#endif
}

Ami::DescEntry* ScalarPlotDesc::desc(const char* title) const
{
  DescEntry* desc = 0;

  QString vn = QString("(%1)/(%2)").arg(title).arg(_vnorm->entry());

  QString qtitle = title ? QString(title) : _title->text();
  std::string w( _weightB->isChecked() ? qPrintable(_vweight->entry()) : "" );

  switch(_plot_grp->currentIndex()) {
  case ScalarPlotDesc::TH1F:
    { QString v = _xnorm->isChecked() ? vn : qtitle;
      std::string sv(qPrintable(v));
      switch(_hist->method()) {
      case DescBinning::Fixed:
        desc = new Ami::DescTH1F(sv.c_str(),sv.c_str(),"events",
                                 _hist->bins(),_hist->lo(),_hist->hi(),
				 _hist->normalize()); 
        break;
      case DescBinning::Auto1:
        desc = new Ami::DescScalarRange(sv.c_str(),"events",
                                        DescScalarRange::MeanSigma,
                                        _hist->sigma(),
                                        _hist->nsamples(),
                                        _hist->bins(),
					_hist->normalize());
        break;
      case DescBinning::Auto2:
        desc = new Ami::DescScalarRange(sv.c_str(),"events",
                                        DescScalarRange::MinMax,
                                        _hist->extent(),
                                        _hist->nsamples(),
                                        _hist->bins(),
					_hist->normalize());
        break;
      default:
        break;
      }
      break; }
  case ScalarPlotDesc::vT: 
    { std::string v( qPrintable(_ynorm->isChecked() ? vn : qtitle) );
      switch(_vTime->stat()) {
      case Ami::DescScalar::StdDev:
        desc = new Ami::DescScalar(v.c_str(),"stddev", Ami::DescScalar::StdDev,
                                   w.c_str(),_vTime->pts(),_vTime->dpt());
        break;
      case Ami::DescScalar::Mean:
      default:
        desc = new Ami::DescScalar(v.c_str(),"mean", Ami::DescScalar::Mean,
                                   w.c_str(),_vTime->pts(),_vTime->dpt());
        break; }
      break; }
  case ScalarPlotDesc::vF:
    { QString vy = _ynorm->isChecked() ? vn : qtitle;
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vFeature->expr()).arg(_vnorm->entry()) : _vFeature->expr();
      desc = new Ami::DescProf(qPrintable(vy),
			       qPrintable(vx),"mean",
			       _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean",
			       DescEntry::Mean,w.c_str());
      break; }
  case ScalarPlotDesc::vF2:
    { QString vy = _ynorm->isChecked() ? QString("(%1)/(%2)").arg(_vFeature2->yexpr()).arg(_vnorm->entry()) : _vFeature2->yexpr();
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vFeature2->xexpr()).arg(_vnorm->entry()) : _vFeature2->xexpr();
      QString vz = qtitle;
      desc = new Ami::DescProf2D(qPrintable(vz),
                                 qPrintable(vx),qPrintable(vy),
                                 _vFeature2->xbins(),_vFeature2->xlo(),_vFeature2->xhi(),
                                 _vFeature2->ybins(),_vFeature2->ylo(),_vFeature2->yhi(),
                                 "mean",
                                 DescEntry::Mean,w.c_str());
      break; }
  case ScalarPlotDesc::vS:
    { QString vy = _ynorm->isChecked() ? vn : qtitle;
      QString vx = _xnorm->isChecked() ? QString("(%1)/(%2)").arg(_vScan->expr()).arg(_vnorm->entry()) : _vScan->expr();
      desc = new Ami::DescScan(qPrintable(vy),
			       qPrintable(_vScan->expr()),title,
			       _vScan->bins(),
			       w.c_str());
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

  case ScalarPlotDesc::vF2:
    if (_vFeature2->xexpr().contains(_post_str))
      post = true;
    if (_vFeature2->yexpr().contains(_post_str))
      post = true;
    if (_xnorm->isChecked() && _vnorm->entry().contains(_post_str))
      post = true;
    if (_ynorm->isChecked() && _vnorm->entry().contains(_post_str))
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

void ScalarPlotDesc::aggClicked(int b)
{
  //
  // Only 0 or 1 buttons may be checked
  //
  if (_agg_grp->button(b)->isChecked()) {
    for(int i=0; i<NAggs; i++)
      if (i!=b)
	_agg_grp->button(i)->setChecked(false);
  }
}

void ScalarPlotDesc::update_interval()
{
  unsigned nproc = SMPRegistry::instance().nservers();
  int n = _interval->text().toInt();
  int m = nproc*avgRound(n,nproc);
  if (n>1 && m!=n)
    _intervalq->setText(QString("(%1)").arg(QString::number(m)));
  else
    _intervalq->clear();
}
