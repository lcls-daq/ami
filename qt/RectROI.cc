#include "ami/qt/RectROI.hh"

#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ProjectionPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/Rect.hh"
#include "ami/qt/AxisBins.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/data/RectROI.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/Entry.hh"

#include <stdio.h>

using namespace Ami::Qt;

RectROI::RectROI(QWidget*         p,
                 const QString&   n,
                 unsigned         ch, 
                 const Rect&      r) : 
  _parent (p),
  _name   (n),
  _channel(ch),
  _signature(-1),
  _rect   (r),
  _list_sem(Semaphore::FULL),
  _roi_requested(false)
{
}

RectROI::~RectROI()
{
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
}

void RectROI::save(char*& p) const
{  
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++) {
    XML_insert(p, "ProjectionPlot", "_pplots", (*it)->save(p) );
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
  XML_insert( p, "bool", "_roi_requested", QtPersistent::insert(p,_roi_requested));
}

void RectROI::load(const char*& p)
{
  _roi_requested = false;

  _list_sem.take();

  for(std::list<ProjectionPlot*>::iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    delete *it;
  _pplots.clear();

  for(std::list<CursorPlot*>::iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    delete *it;
  _cplots.clear();

  for(std::list<ZoomPlot*>::iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    delete *it;
  _zplots.clear();

  for(std::list<CursorPost*>::iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  // mem leak?
  _ovls.clear();

  XML_iterate_open(p,tag)
    if (tag.name == "_pplots") {
      ProjectionPlot* plot = new ProjectionPlot(_parent, p);
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
    else if (tag.name == "_roi_requested")
      _roi_requested = QtPersistent::extract_b(p);
    else if (tag.name == "nil")
      ;
  XML_iterate_close(RectROI,tag);

  _list_sem.give();
}

void RectROI::snapshot(const QString& p) const
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->_snapshot(QString("%1_%2.png").arg(p).arg((*it)->_name));
}

void RectROI::save_plots(const QString& p) const
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->save_plots(QString("%1_%2").arg(p).arg((*it)->_name));
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg((*it)->_name);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void RectROI::configure(char*& p, unsigned input, unsigned& output,
                        ChannelDefinition* channels[], int* input_signatures, unsigned nchannels)
{
  bool smp_prohibit = channels[_channel]->smp_prohibit();

  if (!(_pplots.size() || _cplots.size() || _zplots.size() ||
        _posts.size() || _ovls.size() || _roi_requested)) return;

  if (smp_prohibit && (_pplots.size() || _cplots.size() || _posts.size() ||
		       _ovls.size())) {
    QString s = QString("Plots/posts from %1/%2 disabled [SMP]")
      .arg(channels[_channel]->name())
      .arg(_name);
    ControlLog::instance().appendText(s);
  }

  if (smp_prohibit && !_zplots.size()) return;

  //  Configure the ROI
  Ami::DescImage d(qPrintable(_name),
		   _rect.x1-_rect.x0+1,
		   _rect.y1-_rect.y0+1,
		   1, 1, 
		   _rect.x0, _rect.y0);
  d.aggregate(smp_prohibit);
  Ami::RectROI op(d);
  
  ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Analysis,
                                                    input_signatures[_channel],
                                                    -1,
                                                    RawFilter(),
                                                    op);
  p += req.size();
  _req.request(req, output);
  input = _signature = req.output();

  _list_sem.take();

  //  Configure the derived plots
  if (!smp_prohibit) {
    const unsigned maxpixels=1024;
    AxisBins dummy_axis(0,maxpixels,maxpixels);
    for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
      (*it)->configure(p,input,output);
    for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
    for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
    for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
      (*it)->configure(p,input,output,
		       dummy_axis,Ami::ConfigureRequest::Analysis);
  }
  else {
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
  }

  _list_sem.give();
}

void RectROI::setup_payload(Cds& cds)
{
  _list_sem.take();

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
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

void RectROI::update()
{
  _list_sem.take();

  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();

  _list_sem.give();
}

void RectROI::add_projection(Ami::AbsOperator* op)
{
  ProjectionPlot* plot = 
    new ProjectionPlot(_parent, op->output().name(), _channel, op);
  
  _list_sem.take();
  _pplots.push_back(plot);
  _list_sem.give();

  connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
  connect(plot, SIGNAL(closed(QObject*))  , this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void RectROI::add_cursor_plot(BinMath* op)
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

void RectROI::add_zoom_plot()
{
  ZoomPlot* plot = new ZoomPlot(_parent,
				_name,
                                true);

  _list_sem.take();
  _zplots.push_back(plot);
  _list_sem.give();

  connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

QString RectROI::add_post(const QString& title,
                          const char*    expr)
{
  SharedData* post;
  return add_post(title,expr,post);
}

QString RectROI::add_post(const QString& title,
                          const char*    expr,
                          SharedData*&   shared)
{
  QString qtitle = FeatureRegistry::instance(Ami::PostAnalysis).validate_name(title);

  Ami::DescCache*  desc = new Ami::DescCache(qPrintable(qtitle),
                                             qPrintable(qtitle),
                                             Ami::PostAnalysis);
  CursorPost* post = new CursorPost(_channel,
                                    new BinMath(*desc,expr),
                                    this);
  _posts.push_back(post);

  delete desc;

  emit changed();

  FeatureRegistry::instance(Ami::PostAnalysis).share(qtitle,post);
  shared = post;
  return qtitle;
}

void RectROI::remove_plot(QObject* obj)
{
  _list_sem.take();
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  delete obj;
  _list_sem.give();

  emit changed();
}

void RectROI::add_overlay(DescEntry*,QtPlot*,SharedData*) 
{
}

void RectROI::remove_overlay(QtOverlay* obj)
{
  _list_sem.take();
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _ovls.remove(ovl);
  _list_sem.give();
}

void RectROI::add_overlay(QtPlot& plot, BinMath* op)
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

void RectROI::remove_cursor_post(CursorPost* post)
{
  _list_sem.take();
  _posts.remove(post);
  _list_sem.give();
  emit changed();
}

void RectROI::request_roi(bool v) { _roi_requested=v; }
