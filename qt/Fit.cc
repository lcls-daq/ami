#include "ami/qt/Fit.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/EnvPlot.hh"
#include "ami/qt/EnvPost.hh"
#include "ami/qt/EnvOverlay.hh"
#include "ami/qt/PlotFrame.hh"
#include "ami/qt/PostAnalysis.hh"
#include "ami/qt/QtPlotSelector.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/WaveformDisplay.hh"

#include "ami/data/DescCache.hh"
#include "ami/data/DescEntryW.hh"
#include "ami/data/Fit.hh"
#include "ami/data/FitEntry.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/XML.hh"

#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QValidator>
#include <QtGui/QVBoxLayout>

#include <string>
#include <sstream>

#include <stdlib.h>

using Ami::XML::QtPersistent;
using namespace Ami::Qt;

static Ami::Fit _fit_op(const Ami::AbsOperator& op, const Ami::DescEntry& o)
{
  const Ami::Fit& fit = static_cast<const Ami::Fit&>(op);
  return Ami::Fit(o, fit.function(), fit.parameter());
}

Fit::Fit(QWidget* parent, 
         ChannelDefinition* channels[], 
         unsigned nchannels, 
         WaveformDisplay& frame,
         QtPWidget* frameParent) :
  QtPWidget (parent),
  _channels (channels),
  _nchannels(nchannels),
  _channel  (0),
  _frame    (frame),
  _frameParent(frameParent),
  _list_sem (Semaphore::FULL)
{
  setWindowTitle("Fit");
  setAttribute(::Qt::WA_DeleteOnClose, false);
  
  QComboBox* channelBox = new QComboBox;
  for(unsigned i=0; i<nchannels; i++)
    channelBox->addItem(channels[i]->name());

  _functionBox = new QComboBox;
  for(unsigned i=0; i<Ami::Fit::NumberOf; i++)
    _functionBox->addItem(Ami::Fit::function_str(Ami::Fit::Function(i)));
  _functionBox->setCurrentIndex(0);

  _parameterBox = new QComboBox;
  set_function(_functionBox->currentIndex());
  
  _scalar_desc = new ScalarPlotDesc(0, &FeatureRegistry::instance(), false);

  (_xlo = new QLineEdit)->setMaximumWidth(60);
  (_xhi = new QLineEdit)->setMaximumWidth(60);

  new QDoubleValidator(_xlo);
  new QDoubleValidator(_xhi);

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* closeB = new QPushButton("Close");
  QPushButton* grabB  = new QPushButton("Grab");
  QPushButton* resetB = new QPushButton("Reset");

  QVBoxLayout* layout = new QVBoxLayout;
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Channel"));
    layout1->addWidget(channelBox);
    layout1->addStretch();
    layout1->addWidget(new QLabel("Range"));
    layout1->addWidget(_xlo);
    layout1->addWidget(_xhi);
    layout1->addWidget(grabB);
    layout1->addWidget(resetB);
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Function"));
    layout1->addWidget(_functionBox);
    layout1->addStretch();
    layout1->addWidget(new QLabel("Parameter"));
    layout1->addWidget(_parameterBox); 
    layout->addLayout(layout1); }
  { layout->addWidget(_scalar_desc); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  layout->addStretch();
  setLayout(layout);

  connect(channelBox, SIGNAL(activated(int)), this, SLOT(set_channel(int)));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(closeB    , SIGNAL(clicked()),      this, SIGNAL(closed()));
  connect(grabB     , SIGNAL(clicked()),      this, SLOT(grabRange()));
  connect(resetB    , SIGNAL(clicked()),      this, SLOT(resetRange()));
  connect(_functionBox, SIGNAL(currentIndexChanged(int)), this, SLOT(set_function(int)));

  _plotB = plotB;
  _ovlyB = ovlyB;
  _scalar_desc->post(this, SLOT(add_post()));
}

Fit::~Fit()
{
  _list_sem.take();
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _list_sem.give();
}

void Fit::save(char*& p) const
{
  XML_insert(p, "QtPWidget", "self", QtPWidget::save(p) );

  XML_insert(p, "ScalarPlotDesc", "_scalar_desc", _scalar_desc->save(p) );

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert(p, "EnvPlot", "_plots", (*it)->save(p) );
  }
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "EnvPost", "_posts", (*it)->save(p) );
  }
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert(p, "EnvOverlay", "_ovls", (*it)->save(p) );
  }
}

void Fit::load(const char*& p)
{
  _list_sem.take();

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();

  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  _ovls .clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.name == "_scalar_desc")
      _scalar_desc->load(p);
    else if (tag.name == "_plots") {
      EnvPlot* plot = new EnvPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      EnvPost* post = new EnvPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      EnvOverlay* ovl = new EnvOverlay(*this, p);
      _ovls.push_back(ovl);
    }
  XML_iterate_close(Fit,tag);

  _list_sem.give();

  setVisible(false);

  emit changed();
}

void Fit::save_plots(const QString& p) const
{
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void Fit::configure(char*& p, unsigned input, unsigned& output,
                    ChannelDefinition* ch[], int* signatures, unsigned nchannels,
                    ConfigureRequest::Source source)
{
  _plotB->setEnabled(!_channels[_channel]->smp_prohibit());
  _ovlyB->setEnabled(!_channels[_channel]->smp_prohibit());

  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,signatures[(*it)->channel()],output,source,
                       _fit_op((*it)->op(),(*it)->desc()));
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,signatures[(*it)->channel()],output,source,
                       _fit_op((*it)->op(),(*it)->desc()));
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    if (!_channels[(*it)->channel()]->smp_prohibit())
      (*it)->configure(p,signatures[(*it)->channel()],output,source,
                       _fit_op((*it)->op(),(*it)->desc()));
  _list_sem.give();
}

void Fit::setup_payload(Cds& cds)
{
  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);
  _list_sem.give();
}

void Fit::update()
{
  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  _list_sem.give();
}

void Fit::initialize(const Ami::DescEntry& e)
{
}

void Fit::set_channel   (int c)
{
  _channel=c; 
  _plotB->setEnabled(!_channels[c]->smp_prohibit());
  _ovlyB->setEnabled(!_channels[c]->smp_prohibit());
}

void Fit::set_function (int f)
{
  Ami::FitEntry* e = Ami::FitEntry::instance(Ami::Fit::Function(_functionBox->currentIndex()));
  std::vector<QString> pnames = e->names();
  delete e;
    
  _parameterBox->clear();
  for(unsigned i=0; i<pnames.size(); i++)
    _parameterBox->addItem(pnames[i]);
  _parameterBox->setCurrentIndex(0);
}

void Fit::plot          ()
{
  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    PostAnalysis::instance()->plot(qtitle,entry,_posts.back());
  }
  else {
    DescEntry* desc = _scalar_desc->desc(qPrintable(_parameterBox->currentText()));
    EnvPlot* plot = new EnvPlot(this,
                                _parameterBox->currentText(),
                                Ami::RawFilter(),
                                desc,
                                new Ami::Fit(*desc, 
                                             Ami::Fit::Function(_functionBox->currentIndex()),
                                             _parameterBox->currentIndex(),
                                             _xlo->text().toDouble(),
                                             _xhi->text().toDouble()),
                                _channel);

    _list_sem.take();
    _plots.push_back(plot);
    _list_sem.give();

    connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));
  }
  emit changed();
}

void Fit::overlay       ()
{
  if (_scalar_desc->postAnalysis()) {
    QString     qtitle = _add_post();
    DescEntry*  entry  = _scalar_desc->desc(qPrintable(qtitle));
    new QtPlotSelector(*this, *PostAnalysis::instance(), entry, _posts.back());
  }
  else {
    DescEntry* desc = _scalar_desc->desc(qPrintable(_parameterBox->currentText()));
    new QtPlotSelector(*this, *this, desc);
  }
}

void Fit::remove_plot   (QObject* obj)
{
  EnvPlot* plot = static_cast<EnvPlot*>(obj);
  _list_sem.take();
  _plots.remove(plot);
  _list_sem.give();

  disconnect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;
}

void Fit::add_post      ()
{
  _add_post();
  _posts.back()->signup();
}

QString Fit::_add_post()
{
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(_scalar_desc->qtitle());

  Ami::DescCache* desc = new Ami::DescCache(qPrintable(qtitle),
                                            qPrintable(qtitle),
                                            Ami::PostAnalysis);
  EnvPost* post = new EnvPost(Ami::RawFilter(),
                              desc, 
                              new Ami::Fit(*desc, 
                                           Ami::Fit::Function(_functionBox->currentIndex()),
                                           _parameterBox->currentIndex()),
                              _channel);

  _list_sem.take();
  _posts.push_back(post);
  _list_sem.give();

  emit changed();

  FeatureRegistry::instance(Ami::PostAnalysis).share(qtitle,post);

  return qtitle;
}

void Fit::add_overlay   (DescEntry* desc, QtPlot* plot, SharedData*)
{
  EnvOverlay* ovl = new EnvOverlay(*this, 
                                   *plot,
                                   Ami::RawFilter(),
                                   desc,
                                   new Ami::Fit(*desc, 
                                                Ami::Fit::Function(_functionBox->currentIndex()),
                                                _parameterBox->currentIndex()),
                                   _channel);

  _list_sem.take();
  _ovls.push_back(ovl);
  _list_sem.give();
  
  connect(ovl, SIGNAL(changed()), this, SIGNAL(changed()));
  emit changed();
}
 
void Fit::remove_overlay(QtOverlay* obj)
{
  EnvOverlay* ovl = static_cast<EnvOverlay*>(obj);
  _list_sem.take();
  _ovls.remove(ovl);
  _list_sem.give();
}

void Fit::mousePressEvent(double x, double)
{
  _xlo->setText(QString::number(x));
}

void Fit::mouseMoveEvent(double,double) {}

void Fit::mouseReleaseEvent(double x, double)
{
  double xlo = _xlo->text().toDouble();
  if (x < xlo) {
    _xlo->setText(QString::number(x));
    _xhi->setText(QString::number(xlo));
  }
  else
    _xhi->setText(QString::number(x));
}
 
void Fit::grabRange()
{
  _frame.plot()->set_cursor_input(this);
  if (_frameParent) _frameParent->front();
}

void Fit::resetRange()
{
  _xlo->setText(QString());
  _xhi->setText(QString());
}
