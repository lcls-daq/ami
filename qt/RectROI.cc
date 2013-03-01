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

#include "ami/data/RectROI.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/AbsOperator.hh"
#include "ami/data/BinMath.hh"
#include "ami/data/DescCache.hh"

using namespace Ami::Qt;

RectROI::RectROI(QWidget*         p,
                 const QString&   n,
                 unsigned         ch, 
                 const Rect&      r) : 
  _parent (p),
  _name   (n),
  _channel(ch),
  _rect   (r)
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
  if (!(_pplots.size() || _cplots.size() || _zplots.size() ||
        _posts.size() || _ovls.size())) {
    XML_insert( p, "Nothing", "nil", ; );
    return;
  }

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
}

void RectROI::load(const char*& p)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _pplots.clear();

  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _cplots.clear();

  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _zplots.clear();

  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
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
      connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_cplots") {
      CursorPlot* plot = new CursorPlot(_parent, p);
      _cplots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));
      connect(plot, SIGNAL(changed()), this, SIGNAL(changed()));
    }
    else if (tag.name == "_zplots") {
      ZoomPlot* plot = new ZoomPlot(_parent, p);
      _zplots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
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
}

void RectROI::save_plots(const QString& p) const
{
  int i=1;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->save_plots(QString("%1_%2").arg(p).arg(i++));
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg(i++);
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
  if (!(_pplots.size() || _cplots.size() || _zplots.size() ||
        _posts.size() || _ovls.size())) return;

  //  Configure the ROI
  Ami::RectROI op(Ami::DescImage(qPrintable(_name),
                                 _rect.x1-_rect.x0,
                                 _rect.y1-_rect.y0,
                                 1, 1, 
                                 _rect.x0, _rect.y0));
  
  ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Analysis,
                                                    input_signatures[_channel],
                                                    -1,
                                                    RawFilter(),
                                                    op);
  p += req.size();
  _req.request(req, output);
  input = req.output();

  //  Configure the derived plots
  const unsigned maxpixels=1024;
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->configure(p,input,output);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->configure(p,input,output,
		     AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->configure(p,input,output);
  for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    (*it)->configure(p,input,output,
		     AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->configure(p,input,output,
		     AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
}

void RectROI::setup_payload(Cds& cds)
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
   (*it)->setup_payload(cds);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
   (*it)->setup_payload(cds);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
   (*it)->setup_payload(cds);
}

void RectROI::update()
{
  for(std::list<ProjectionPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
}

void RectROI::add_projection(Ami::AbsOperator* op)
{
  ProjectionPlot* plot = 
    new ProjectionPlot(_parent, op->output().name(), _channel, op);
  
  _pplots.push_back(plot);

  connect(plot, SIGNAL(description_changed()), this, SIGNAL(changed()));
  connect(plot, SIGNAL(destroyed(QObject*))  , this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void RectROI::add_cursor_plot(BinMath* op)
{
  CursorPlot* cplot =
    new CursorPlot(_parent, op->output().name(), _channel, op);

  _cplots.push_back(cplot);
  connect(cplot, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(cplot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  emit changed();
}

void RectROI::add_zoom_plot()
{
  ZoomPlot* plot = new ZoomPlot(_parent,
				_name);
  _zplots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

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

  shared = post;
  return qtitle;
}

void RectROI::remove_plot(QObject* obj)
{
  { ProjectionPlot* plot = static_cast<ProjectionPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void RectROI::add_overlay(DescEntry*,QtPlot*,SharedData*) 
{
}

void RectROI::remove_overlay(QtOverlay* obj)
{
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _ovls.remove(ovl);
}

void RectROI::add_overlay(QtPlot& plot, BinMath* op)
{
  CursorOverlay* ovl = new CursorOverlay(*this, 
                                         plot,
                                         _channel, 
                                         op);
  _ovls.push_back(ovl);

  emit changed();
}

void RectROI::remove_cursor_post(CursorPost* post)
{
  _posts.remove(post);
  emit changed();
}

