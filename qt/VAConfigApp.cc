#include "ami/qt/VAConfigApp.hh"
#include "ami/qt/PeakPlot.hh"
#include "ami/qt/ZoomPlot.hh"
#include "ami/qt/CursorPlot.hh"
#include "ami/qt/CursorPost.hh"
#include "ami/qt/CursorOverlay.hh"
#include "ami/qt/ChannelDefinition.hh"
#include "ami/qt/ControlLog.hh"
#include "ami/qt/AxisBins.hh"

#include "ami/data/BinMath.hh"
#include "ami/data/Cds.hh"
#include "ami/data/Entry.hh"
#include "ami/data/RawFilter.hh"

using namespace Ami::Qt;

VAConfigApp::VAConfigApp(QWidget* parent, 
			 const QString& name, 
			 unsigned i) :
  _parent   (parent),
  _name     (name),
  _channel  (i),
  _signature(-1)
{
}

VAConfigApp::~VAConfigApp() 
{
}

void VAConfigApp::load(const char*& p)
{
  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
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
      PeakPlot* plot = new PeakPlot(_parent, p);
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

void VAConfigApp::save(char*& p) const
{
  if (!(_pplots.size() || _cplots.size() || _zplots.size() ||
        _posts.size() || _ovls.size())) {
    XML_insert( p, "Nothing", "nil", ; );
    return;
  }

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

  Ami::AbsOperator* o = _op(qPrintable(_name));
  
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

  //  Configure the derived plots
  if (!smp_prohibit) {
    const unsigned maxpixels=1024;
    for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
      (*it)->configure(p,input,output,
		       AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
    for(std::list<CursorPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
      (*it)->configure(p,input,output,
		       AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
    for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
      (*it)->configure(p,input,output,
		       AxisBins(0,maxpixels,maxpixels),Ami::ConfigureRequest::Analysis);
    for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
      (*it)->configure(p,input,output);
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
  }
  else {
    for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
      (*it)->configure(p,input,output);
  }
#ifdef DBUG
  printf("VAConfigApp::configure output %d\n", output);
#endif

}

void VAConfigApp::setup_payload(Cds& cds) 
{
  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->setup_payload(cds);
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(cds);

  Entry* entry = cds.entry(_signature);
  if (entry && entry->desc().type() != DescEntry::ScalarRange && _zplots.empty())
    cds.request(*entry, false);
}

void VAConfigApp::update() 
{
  for(std::list<PeakPlot*>::const_iterator it=_pplots.begin(); it!=_pplots.end(); it++)
    (*it)->update();
  for(std::list<CursorPlot*>::const_iterator it=_cplots.begin(); it!=_cplots.end(); it++)
    (*it)->update();
  for(std::list<ZoomPlot*>::const_iterator it=_zplots.begin(); it!=_zplots.end(); it++)
    (*it)->update();
  for(std::list<CursorOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
}

void VAConfigApp::add_map(Ami::AbsOperator* op)
{
#if 1
  PeakPlot* plot = new PeakPlot(_parent,
				_name,
				_channel, op,
				false);
  _pplots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
#endif
  emit changed();
}

void VAConfigApp::add_cursor_plot(BinMath* op)
{
  CursorPlot* cplot =
    new CursorPlot(_parent, op->output().name(), _channel, op);

  _cplots.push_back(cplot);
  connect(cplot, SIGNAL(changed()), this, SIGNAL(changed()));
  connect(cplot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  emit changed();
}

void VAConfigApp::remove_plot(QObject* obj)
{
  { PeakPlot* plot = static_cast<PeakPlot*>(obj);
    _pplots.remove(plot); }

  { CursorPlot* plot = static_cast<CursorPlot*>(obj);
    _cplots.remove(plot); }

  { ZoomPlot* plot = static_cast<ZoomPlot*>(obj);
    _zplots.remove(plot); }

  disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void VAConfigApp::add_overlay(QtPlot& plot, BinMath* op) 
{
  CursorOverlay* ovl = new CursorOverlay(*this, 
                                         plot,
                                         _channel, 
                                         op);
  _ovls.push_back(ovl);

  emit changed();
}

void VAConfigApp::add_overlay(DescEntry*,QtPlot*,SharedData*)
{
}

void VAConfigApp::remove_overlay(QtOverlay* obj)
{
  CursorOverlay* ovl = static_cast<CursorOverlay*>(obj);
  _ovls.remove(ovl);
}

