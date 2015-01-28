#include "LineFitClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/FeatureCalculator.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/EnvPlot.hh"
#include "ami/qt/EnvOverlay.hh"
#include "ami/qt/EnvTable.hh"
#include "ami/qt/LineFitPlotDesc.hh"
#include "ami/qt/QtPlot.hh"
#include "ami/qt/QtPlotSelector.hh"
#include "ami/qt/SharedData.hh"
#include "ami/qt/ControlLog.hh"

#include "ami/app/XtcClient.hh"
#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/RawFilter.hh"
#include "ami/data/LineFit.hh"

#include "ami/service/Socket.hh"
#include "ami/service/Semaphore.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>

#include <sys/types.h>
#include <sys/socket.h>

using namespace Ami::Qt;

static const int BufferSize = 0x8000;

static Ami::LineFit _linefit(const Ami::DescEntry&);

LineFitClient::LineFitClient(QWidget* parent, const Pds::DetInfo& info, unsigned channel, const QString& name) :
  Ami::Qt::AbsClient(parent,info,channel),
  _title           (name),
  _input           (0),
  _set             (Ami::PostAnalysis),
  _output_signature(0),
  _request         (new char[BufferSize]),
  _description     (new char[BufferSize]),
  _cds             ("Client"),
  _manager         (0),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _sem             (new Semaphore(Semaphore::EMPTY)),
  _throttled       (false),
  _list_sem        (Semaphore::FULL),
  _reset           (false)
{
  setWindowTitle(QString("%1[*]").arg(_title));
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this,2.5);
  _status  = new Status;

  QPushButton* filterB = new QPushButton("Filter");
  _filter = new Filter     (this,_title);

  _xsource_edit    = new QLineEdit("");
  _xsource_compose = new QPushButton("Select");
  _ysource_edit    = new QLineEdit("");
  _ysource_compose = new QPushButton("Select");

  _scalar_plot = new LineFitPlotDesc(this, &FeatureRegistry::instance(_set));

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* ovlyB  = new QPushButton("Overlay");
  QPushButton* tablB  = new QPushButton("Table");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_control);
    layout1->addStretch();
    layout1->addWidget(_status);
    layout->addLayout(layout1); }
  { QGroupBox* channel_box = new QGroupBox("Source Channels");
    QGridLayout* layout1 = new QGridLayout;
    layout1->addWidget(new QLabel("X:"),0,0);
    layout1->addWidget(_xsource_edit   ,0,1);
    layout1->addWidget(_xsource_compose,0,2);
    layout1->addWidget(new QLabel("Y:"),1,0);
    layout1->addWidget(_ysource_edit   ,1,1);
    layout1->addWidget(_ysource_compose,1,2);
    layout1->addWidget(filterB         ,0,3,2,1);
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { layout->addWidget(_scalar_plot); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(ovlyB);
    layout1->addWidget(tablB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(filterB   , SIGNAL(clicked()),   _filter, SLOT(front()));
  connect(_xsource_compose, SIGNAL(clicked()), this, SLOT(select_xsource()));
  connect(_ysource_compose, SIGNAL(clicked()), this, SLOT(select_ysource()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(tablB     , SIGNAL(clicked()),      this, SLOT(table()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect((AbsClient*)this, SIGNAL(changed()), this, SLOT(update_configuration()));
}

LineFitClient::~LineFitClient() 
{
  printf("====== destroying LineFitClient ========\n");

  if (_manager) delete _manager;
  delete[] _iovload;
  delete[] _description;
  delete[] _request;
  delete _filter;
}

const QString& LineFitClient::title() const { return _title; }

void LineFitClient::save(char*& p) const
{
  XML_insert( p, "QtPWidget", "self", 
              QtPWidget::save(p) );
  XML_insert( p, "QLineEdit", "_xsource_edit", QtPersistent::insert(p,_xsource_edit->text()) );
  XML_insert( p, "QLineEdit", "_ysource_edit", QtPersistent::insert(p,_ysource_edit->text()) );
  XML_insert( p, "Filter", "_filter",
              _filter->save(p) );

  XML_insert( p, "ScalarPlotDesc", "_scalar_plot",
              _scalar_plot->save(p) );

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert( p, "EnvPlot", "_plots",
                (*it)->save(p) );
  }
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++) {
    XML_insert( p, "EnvOverlay", "_ovls",
                (*it)->save(p) );
  }
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++) {
    XML_insert( p, "EnvTable", "_tabls",
                (*it)->save(p) );
  }

  XML_insert( p, "Control", "_control", 
              _control->save(p) );
}

void LineFitClient::load(const char*& p)
{
  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();
  // mem leak?
  _ovls.clear();
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++) {
    disconnect(*it, SIGNAL(remove(QObject*)), this, SLOT(remove_table(QObject*)));
    delete *it;
  }
  _tabls.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.element == "Filter")
      _filter  ->load(p);
    else if (tag.element == "ScalarPlotDesc")
      _scalar_plot->load(p);
    else if (tag.element == "EnvPlot") {
      EnvPlot* plot = new EnvPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_xsource_edit")
      _xsource_edit->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_ysource_edit")
      _ysource_edit->setText(QtPersistent::extract_s(p));
    else if (tag.name == "_ovls") {
      EnvOverlay* ovl = new EnvOverlay(*this, p);
      _ovls.push_back(ovl);
    }
    else if (tag.name == "_tabls") {
      EnvTable* tabl = new EnvTable(this, p);
      _tabls.push_back(tabl);
      connect(tabl, SIGNAL(remove(QObject*)), this, SLOT(remove_table(QObject*)));
    }
    else if (tag.element == "Control")
      _control->load(p);
  XML_iterate_close(LineFitClient,tag);

  _list_sem.give();

  update_configuration();
}

void LineFitClient::save_plots(const QString& p) const 
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

void LineFitClient::reset_plots() 
{ 
  _reset=true;
  update_configuration(); 
}

void LineFitClient::connected()
{
  if (_manager) {
    _status->set_state(Status::Connected);
    _manager->discover();
  }
}

void LineFitClient::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);

  const DescEntry* e = rx.entry(Ami::EntryScalar::input_entry());
  if (e) {
    _input = e->signature();
  }
  else {
    printf("LineFitClient failed to find input\n");
    _input = -1;

    rx.dump();
  }

  _manager->configure();
}

int  LineFitClient::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);

  char* p = _request;

  if (_reset) {
    _reset=false;
    ConfigureRequest& req = *new (p) ConfigureRequest(ConfigureRequest::Reset,
						      ConfigureRequest::Analysis,
						      _input);
    p += req.size();
  }
  else {
    ConfigureRequest::Source s(ConfigureRequest::Discovery);
    _list_sem.take();
    for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
      (*it)->configure(p,_input,_output_signature,s, _linefit((*it)->desc()));
    for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
      (*it)->configure(p,_input,_output_signature,s, _linefit((*it)->desc()));
    for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++)
      (*it)->configure(p,_input,_output_signature,s, _linefit((*it)->desc()));
    _list_sem.give();
  }
  if (p > _request+BufferSize) {
    printf("Client request overflow: size = 0x%x\n", (unsigned) (p-_request));
    return 0;
  }
  else {
    iov[0].iov_base = _request;
    iov[0].iov_len  = p - _request;
    return 1;
  }
}

int  LineFitClient::configured      () 
{
  return 0; 
}

int  LineFitClient::read_description(Socket& socket,int len)
{
  int size = socket.read(_description,len);

  if (size<0) {
    printf("Read error in Ami::Qt::Client::read_description.\n");
    return 0;
  }

  if (size==BufferSize) {
    printf("Buffer overflow in Ami::Qt::Client::read_description.  Dying...\n");
    abort();
  }

  //  printf("emit description\n");
  emit description_changed(size);

  _sem->take();

  return size;
}

void LineFitClient::_read_description(int size)
{
  _cds.reset();

  const char* payload = _description;
  const char* const end = payload + size;
  //  dump(payload, size);

  //  { const Desc* e = reinterpret_cast<const Desc*>(payload);
  //    printf("Cds %s\n",e->name()); }
  payload += sizeof(Desc);

  while( payload < end ) {
    const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
    if (desc->size()==0) {
      printf("read_description size==0\n");
      break;
    }
    Entry* entry = EntryFactory::entry(*desc);
    _cds.add(entry, desc->signature());
    payload += desc->size();
//     printf("Received desc %s signature %d\n",desc->name(),desc->signature());
  }

  if (_cds.totalentries()>_niovload) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=_cds.totalentries()];
  }
  _cds.payload(_iovload);

  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(_cds);
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(_cds);
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++) {
    (*it)->setup_payload(_cds);
  }
  _list_sem.give();

  _status->set_state(Status::Described);

  _sem->give();
}

int  LineFitClient::read_payload     (Socket& socket,int)
{
  int nbytes = 0;
  //  printf("payload\n"); 
  if (_status->state() == Status::Requested) {
    nbytes = socket.readv(_iovload,_cds.totalentries());
  }
  else {
    //
    //  Add together each server's contribution
    //
    printf("Ami::Qt::Client::read_payload: multiple server processing not implemented\n");
  }
  _status->set_state(Status::Received);
  return nbytes;
}

bool LineFitClient::svc             () const { return true; }

void LineFitClient::process         () 
{
  //
  //  Perform client-side processing
  //
  _list_sem.take();
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++)
    (*it)->update();
  _list_sem.give();
  
  _status->set_state(Status::Processed);
}

void LineFitClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  _manager->connect();
}

void LineFitClient::request_payload()
{
  if (_plots.size()==0 && _ovls.size()==0 && _tabls.size()==0) return;

  if (_status->state() == Status::Described ||
      _status->state() == Status::Processed) {
    _throttled = false;
    _status->set_state(Status::Requested);
    _manager->request_payload();
  }
  else if (_status->state() == Status::Requested &&
           !_throttled) {
    _status->set_state(Status::Throttled);
    _throttled = true;
    QString msg = QString("%1 request payload throttling").arg(_title);
    printf("%s\n",qPrintable(msg));
    ControlLog::instance().appendText(msg);
  }
}

void LineFitClient::one_shot        (bool) {}

void LineFitClient::update_configuration()
{
  if (_manager)
    _manager->configure();
}

void LineFitClient::plot()
{
  std::string title = Ami::LineFit::title( qPrintable(_xsource_edit->text()),
					   qPrintable(_ysource_edit->text()),
					   qPrintable(_scalar_plot->stat()));
  QString qtitle(title.c_str());

  DescEntry* desc = _scalar_plot->desc(title.c_str());

  EnvPlot* plot = new EnvPlot(this,
			      qtitle,
			      *_filter->filter(),
			      desc,
                              _set);

  _list_sem.take();
  _plots.push_back(plot);
  _list_sem.give();

  connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  connect(plot, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));

  emit changed();
}

void LineFitClient::overlay()
{
  std::string title = Ami::LineFit::title( qPrintable(_xsource_edit->text()),
					   qPrintable(_ysource_edit->text()),
					   qPrintable(_scalar_plot->stat()));

  DescEntry* desc = _scalar_plot->desc(title.c_str());

  new QtPlotSelector(*this, *this, desc);
}

void LineFitClient::table()
{
  std::string title = Ami::LineFit::title( qPrintable(_xsource_edit->text()),
					   qPrintable(_ysource_edit->text()),
					   qPrintable(_scalar_plot->stat()));

  DescEntry* desc = _scalar_plot->table(title.c_str());

  EnvTable* tabl = new EnvTable(this,
                                *_filter->filter(),
                                static_cast<Ami::DescScalar*>(desc),
                                _set);
  _list_sem.take();
  _tabls.push_back(tabl);
  _list_sem.give();

  connect(tabl, SIGNAL(remove(QObject*)), this, SLOT(remove_table(QObject*)));
  emit changed();
}

void LineFitClient::remove_plot(QObject* obj)
{
  EnvPlot* plot = static_cast<EnvPlot*>(obj);
  _list_sem.take();
  _plots.remove(plot);
  _list_sem.give();

  delete plot;

  emit changed();
}

void LineFitClient::remove_table(QObject* obj)
{
  EnvTable* tabl = static_cast<EnvTable*>(obj);
  _list_sem.take();
  _tabls.remove(tabl);
  _list_sem.give();

  delete tabl;
  emit changed();
}


void LineFitClient::select_xsource() { _select_source(_xsource_edit); }
void LineFitClient::select_ysource() { _select_source(_ysource_edit); }

void LineFitClient::_select_source(QLineEdit* e) 
{
  FeatureCalculator* c = new FeatureCalculator(this, "Source", FeatureRegistry::instance(_set));
  if (c->exec()==QDialog::Accepted) {
    e->setText(c->result());
  }
  delete c;
}

void LineFitClient::plot(const QString& name, 
			 DescEntry*     desc, 
			 SharedData*    shared)
{
  EnvPlot* plot = new EnvPlot(this,
			      name,
			      Ami::RawFilter(),
			      desc,
                              _set,
                              shared);

  _list_sem.take();
  _plots.push_back(plot);
  _list_sem.give();

  connect(plot, SIGNAL(closed(QObject*)), this, SLOT(remove_plot(QObject*)));
  connect(plot, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));

  emit changed();
}

void LineFitClient::add_overlay(DescEntry*  desc,
				QtPlot*     plot,
				SharedData* shared)
{
  EnvOverlay* ovl = new EnvOverlay(*this,
                                   *plot,
                                   *_filter->filter(),
                                   desc,
                                   _set,
                                   shared);
                                     
  _list_sem.take();
  _ovls.push_back(ovl);
  _list_sem.give();
  connect(ovl, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));
  
  emit changed();
}

void LineFitClient::remove_overlay(QtOverlay* obj)
{
  EnvOverlay* ovl = static_cast<EnvOverlay*>(obj);
  _list_sem.take();
  _ovls.remove(ovl);
  _list_sem.give();

  //  emit changed();
}

Ami::LineFit _linefit(const Ami::DescEntry& o)
{
  const Ami::DescEntryW& w = static_cast<const Ami::DescEntryW&>(o);
  return Ami::LineFit(o,Ami::LineFit::method(w.weight()));
}
