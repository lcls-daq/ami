#include "ami/qt/VAConfigApp.hh"
#include "ami/qt/PeakPlot.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ControlLog.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/FeatureRegistry.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/Cds.hh"
#include "ami/data/Entry.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/DescCache.hh"

#include <stdio.h>
#include <stdlib.h>

//#define DBUG

using namespace Ami::Qt;

VAConfigApp::VAConfigApp(QWidget* parent, 
			 const QString& name, 
			 unsigned i) :
  _parent   (parent),
  _name     (name),
  _channel  (i),
  _signature(-1),
  _list_sem (Semaphore::FULL)
{
}

VAConfigApp::VAConfigApp(QWidget* parent, 
			 const char*& p) :
  _parent   (parent),
  _signature(-1),
  _list_sem (Semaphore::FULL)
{
  _list_sem.take();

  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _cplots.clear();

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _zplots.clear();

  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  // mem leak?
  _ovls.clear();

  XML_iterate_open(p,tag)
    if (tag.name == "_name") {
      _name = QtPersistent::extract_s(p);
    }
    else if (tag.name == "_channel") {
      _channel = QtPersistent::extract_i(p);
    }
    else if (tag.name == "_pplots") {
      PeakPlot* plot = new PeakPlot(_parent, p);
      _pplots.push_back(plot);
      connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
      connect(plot, SIGNAL(closed(QObject*))  , this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_cplots") {
      CursorPlot* plot = new CursorPlot(_parent, p);
      _cplots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*))  , this, SLOT(remove_plot(QObject*)));
      connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));
    }
    else if (tag.name == "_zplots") {
      ZoomPlot* plot = new ZoomPlot(_parent, p);
      _zplots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      CursorPost* post = new CursorPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      CursorOverlay* ovl = new CursorOverlay(*this, p);
      _ovls.push_back(ovl);
    }
    else if (tag.name == "nil")
      ;
  XML_iterate_close(RectROI,tag);

  _list_sem.give();
}

VAConfigApp::~VAConfigApp() 
{
}

void VAConfigApp::save(char*& p) const
{
  XML_insert(p, "QString" , "_name"   , QtPersistent::insert(p,_name));
  XML_insert(p, "unsigned", "_channel", QtPersistent::insert(p,_channel));

  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    XML_insert(p, "PeakPlot", "_pplots", (*it)->save(p) );
  }

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    XML_insert(p, "CursorPlot", "_cplots", (*it)->save(p) );
  }

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++) {
    XML_insert(p, "ZoomPlot", "_zplots", (*it)->save(p) );
  }
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "CursorPost", "_posts", (*it)->save(p) );
  }
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert( p, "CursorOverlay", "_ovls",
                (*it)->save(p) );
  }
}

void VAConfigApp::save_plots(const QString& p) const
{
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void VAConfigApp::configure(char*& p, unsigned input, unsigned& output,
			    ChannelDefinition* channels[], int* signatures, unsigned nchannels) 
{
  bool smp_prohibit = channels[_channel]->smp_prohibit();

#ifdef DBUG
  printf("VAConfigApp::configure input %d  pplots %zd  cplots %zd  zplots %zd  smp %c\n",
	 input, _pplots.size(), _cplots.size(), _zplots.size(), smp_prohibit ? 't':'f');
#endif

  if (!(_pplots.size() || _cplots.size() || _zplots.size() ||
        _posts.size() || _ovls.size())) return;

  if (smp_prohibit && (_pplots.size() || _cplots.size() || _posts.size() ||
		       _ovls.size())) {
    QString s = QString("Plots/posts from %1/%2 disabled [SMP]")
      .arg(channels[_channel]->name())
      .arg(_name);
    ControlLog::instance().appendText(s);
  }

  if (smp_prohibit && !_zplots.size()) return;

  Ami::AbsOperator* o = _op(_name.toAscii().constData());
  
  ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Analysis,
                                                    signatures[_channel],
                                                    -1,
                                                    RawFilter(),
                                                    *o);
  delete o;

  p += req.size();
  _req.request(req, output);
  input = _signature = req.output();

#ifdef DBUG
  printf("VAConfigApp::configure plots input %d\n", input);
#endif

  _list_sem.take();

  //  Configure the derived plots
  if (!smp_prohibit) {
    const unsigned maxpixels=1024;
    AxisBins dummy_axis(0,maxpixels,maxpixels);
    for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
    for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
    for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
    for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
      (*it)->configure(p,input,output);
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
  }
  else {
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
  }

  _list_sem.give();

#ifdef DBUG
  printf("VAConfigApp::configure output %d\n", output);
#endif

}

void VAConfigApp::setup_payload(Cds& cds) 
{
  _list_sem.take();
  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);
  _list_sem.give();

  Entry* entry = cds.entry(_signature);
  if (entry && entry->desc().type() != DescEntry::ScalarRange && _zplots.empty())
    cds.request(*entry, false);
}

void VAConfigApp::update() 
{
  _list_sem.take();
  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  _list_sem.give();
}

void VAConfigApp::add_map(Ami::AbsOperator* op)
{
#if 1
  PeakPlot* plot = new PeakPlot(_parent,
				_name,
				_channel, op,
				false);
  _list_sem.take();
  _pplots.push_back(plot);
  _list_sem.give();

  connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
  connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
#endif
  emit changed();
}

void VAConfigApp::add_cursor_plot(BinMath* op)
{
  CursorPlot* cplot =
    new CursorPlot(_parent, op->output().name(), _channel, op);

  _list_sem.take();
  _cplots.push_back(cplot);
  _list_sem.give();

  connect(cplot, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(cplot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  emit changed();
}

QString VAConfigApp::add_post(const QString& title,
			      const char*    expr,
			      SharedData*&   shared)
{
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(title);

  Ami::DescCache*  desc = new Ami::DescCache(qPrintable(qtitle),
                                             qPrintable(qtitle),
                                             Ami::PostAnalysis);
  
  CursorPost* post = new CursorPost(0,
				    new BinMath(*desc,expr),
				    this);
  _posts.push_back(post);

  delete desc;

  emit changed();

  FeatureRegistry::instance(Ami::PostAnalysis).share(qtitle,post);
  shared = post;
  return qtitle;
}

void VAConfigApp::remove_plot(QObject* obj)
{
  _list_sem.take();
  { PeakPlot* plot = static_cast<PeakPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }
  _list_sem.give();

  disconnect(obj, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;

  emit changed();
}

void VAConfigApp::add_overlay(QtPlot& plot, BinMath* op) 
{
  CursorOverlay* ovl = new CursorOverlay(*this, 
                                         plot,
                                         _channel, 
                                         op);
  _list_sem.take();
  _ovls.push_back(ovl);
  _list_sem.give();

  connect(ovl, SIGNAL(changed()), this, SIGNAL(changed()));
  emit changed();
}

void VAConfigApp::add_overlay(DescEntry*,QtPlot*,SharedData*)
{
}

void VAConfigApp::remove_overlay(QtOverlay* obj)
{
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _list_sem.take();
  _ovls.remove(ovl);
  _list_sem.give();
}

void VAConfigApp::remove_cursor_post(CursorPost* post)
{
  _list_sem.take();
  _posts.remove(post);
  _list_sem.give();
  emit changed();
}

