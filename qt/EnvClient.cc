#include "EnvClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/FeatureCalculator.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/Filter.hh"
#include "ami/qt/EnvPlot.hh"
#include "ami/qt/EnvPost.hh"
#include "ami/qt/EnvOverlay.hh"
#include "ami/qt/EnvTable.hh"
#include "ami/qt/ScalarPlotDesc.hh"
#include "ami/qt/QtPlot.hh"
#include "ami/qt/QtPlotSelector.hh"

#include "ami/app/XtcClient.hh"
#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/DescCache.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/RawFilter.hh"

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

EnvClient::EnvClient(QWidget* parent, const Pds::DetInfo& info, unsigned channel) :
  Ami::Qt::AbsClient(parent,info,channel),
  _input           (0),
  _set             (Ami::ScalarSet(channel)),
  _output_signature(0),
  _request         (new char[BufferSize]),
  _description     (new char[BufferSize]),
  _cds             ("Client"),
  _manager         (0),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _sem             (new Semaphore(Semaphore::EMPTY)),
  _throttled       (false)
{
#if 0
  switch(_set) {
  case Ami::PreAnalysis : _title = QString("Env"); break;
  case Ami::PostAnalysis: _title = QString("PostAnalysis"); break;
  default:                _title = QString("Environment"); break;
  }
#else
  _title = QString("Env");
#endif
  setWindowTitle(_title);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this,2.5);
  _status  = new Status;

  QPushButton* filterB = new QPushButton("Filter");
  _filter = new Filter     (this,_title);

  _source_edit    = new QLineEdit("");
  _source_compose = new QPushButton("Select");

  _scalar_plot = new ScalarPlotDesc(this, &FeatureRegistry::instance(_set));

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
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_source_edit);
    layout1->addWidget(_source_compose);
    layout1->addWidget(filterB);
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
  connect(_source_edit   , SIGNAL(editingFinished()), this, SLOT(validate_source()));
  connect(_source_compose, SIGNAL(clicked()),         this, SLOT(select_source()));
  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(ovlyB     , SIGNAL(clicked()),      this, SLOT(overlay()));
  connect(tablB     , SIGNAL(clicked()),      this, SLOT(table()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect((AbsClient*)this, SIGNAL(changed()), this, SLOT(update_configuration()));

#if 1
  //  Is this dangerous?
  if (_set!=Ami::PostAnalysis)
    _scalar_plot->post(this, SLOT(add_post()));
#endif
}

EnvClient::~EnvClient() 
{
  printf("====== destroying EnvClient ========\n");

  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();

  if (_manager) delete _manager;
  delete[] _iovload;
  delete[] _description;
  delete[] _request;
  delete _filter;
}

const QString& EnvClient::title() const { return _title; }

void EnvClient::save(char*& p) const
{
  XML_insert( p, "QtPWidget", "self", 
              QtPWidget::save(p) );
  XML_insert( p, "QLineEdit", "_source_edit", 
              QtPersistent::insert(p,_source_edit->text()) );
  XML_insert( p, "Filter", "_filter",
              _filter->save(p) );

  XML_insert( p, "ScalarPlotDesc", "_scalar_plot",
              _scalar_plot->save(p) );

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    XML_insert( p, "EnvPlot", "_plots",
                (*it)->save(p) );
  }
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    XML_insert(p, "EnvPost", "_posts", (*it)->save(p) );
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

void EnvClient::load(const char*& p)
{
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    delete *it;
  }
  _plots.clear();
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++) {
    delete *it;
  }
  _posts.clear();
  // mem leak?
  _ovls.clear();
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++) {
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_table(QObject*)));
    delete *it;
  }
  _tabls.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "QtPWidget")
      QtPWidget::load(p);
    else if (tag.element == "QLineEdit")
      _source_edit->setText(QtPersistent::extract_s(p));
    else if (tag.element == "Filter")
      _filter  ->load(p);
    else if (tag.element == "ScalarPlotDesc")
      _scalar_plot->load(p);
    else if (tag.element == "EnvPlot") {
      EnvPlot* plot = new EnvPlot(this, p);
      _plots.push_back(plot);
      connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
    }
    else if (tag.name == "_posts") {
      EnvPost* post = new EnvPost(p);
      _posts.push_back(post);
    }
    else if (tag.name == "_ovls") {
      EnvOverlay* ovl = new EnvOverlay(*this, p);
      _ovls.push_back(ovl);
    }
    else if (tag.name == "_tabls") {
      EnvTable* tabl = new EnvTable(this, p);
      _tabls.push_back(tabl);
      connect(tabl, SIGNAL(destroyed(QObject*)), this, SLOT(remove_table(QObject*)));
    }
    else if (tag.element == "Control")
      _control->load(p);
  XML_iterate_close(EnvClient,tag);

  update_configuration();
}

void EnvClient::save_plots(const QString& p) const 
{
  int i=1;
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QString s = QString("%1_%2.dat").arg(p).arg(i++);
    FILE* f = fopen(qPrintable(s),"w");
    if (f) {
      (*it)->dump(f);
      fclose(f);
    }
  }
}

void EnvClient::reset_plots() 
{ 
  _input++;
  iovec iov;
  configure(&iov);

  _input--;
  update_configuration(); 
}

void EnvClient::connected()
{
  if (_manager) {
    _status->set_state(Status::Connected);
    _manager->discover();
  }
}

void EnvClient::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);

  const DescEntry* e = rx.entry(Ami::EntryScalar::input_entry());
  if (e)
    _input = e->signature();
  else {
    printf("EnvClient failed to find input\n");
    _input = -1;
  }

  //  iterate through discovery and print
#if 0
  //  FeatureRegistry::instance(_set).insert(rx.features(_set));
  for(int i=0; i<Ami::NumberOfSets; i++) {
    Ami::ScalarSet set((Ami::ScalarSet)i);
    FeatureRegistry::instance(set).insert(rx.features(set));
  }
#endif

  _manager->configure();
}

int  EnvClient::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);

  char* p = _request;

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,_input,_output_signature);
  for(std::list<EnvPost*>::const_iterator it=_posts.begin(); it!=_posts.end(); it++)
    (*it)->configure(p,_input,_output_signature);
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->configure(p,_input,_output_signature);
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++)
    (*it)->configure(p,_input,_output_signature);

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

int  EnvClient::configured      () 
{
  return 0; 
}

int  EnvClient::read_description(Socket& socket,int len)
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

void EnvClient::_read_description(int size)
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

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->setup_payload(_cds);
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->setup_payload(_cds);
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++)
    (*it)->setup_payload(_cds);

  _status->set_state(Status::Described);

  _sem->give();
}

int  EnvClient::read_payload     (Socket& socket,int)
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

bool EnvClient::svc             () const { return true; }

void EnvClient::process         () 
{
  //
  //  Perform client-side processing
  //
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  for(std::list<EnvOverlay*>::const_iterator it=_ovls.begin(); it!=_ovls.end(); it++)
    (*it)->update();
  for(std::list<EnvTable*>::const_iterator it=_tabls.begin(); it!=_tabls.end(); it++)
    (*it)->update();
  
  _status->set_state(Status::Processed);
}

void EnvClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  _manager->connect();
}

void EnvClient::request_payload()
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
    _throttled = true;
    printf("%s request_payload throttling\n",qPrintable(_title));
  }
}

void EnvClient::one_shot        (bool) {}

void EnvClient::update_configuration()
{
  if (_manager)
    _manager->configure();
}

void EnvClient::plot()
{
  QString entry(_source_edit->text());
  DescEntry* desc = _scalar_plot->desc(qPrintable(entry));

  EnvPlot* plot = new EnvPlot(this,
			      entry,
			      *_filter->filter(),
			      desc,
                              _set);

  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  connect(plot, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));

  emit changed();
}

void EnvClient::overlay()
{
  QString entry(_source_edit->text());
  DescEntry* desc = _scalar_plot->desc(qPrintable(entry));

  new QtPlotSelector(*this, *this, desc);
}

void EnvClient::table()
{
  QString entry(_source_edit->text());
  DescScalar* desc = new Ami::DescScalar(qPrintable(entry),
                                         "mean", Ami::DescScalar::Mean, "",
                                         1, 1);
  EnvTable* tabl = new EnvTable(this,
                                *_filter->filter(),
                                desc,
                                _set);
  _tabls.push_back(tabl);
  connect(tabl, SIGNAL(destroyed(QObject*)), this, SLOT(remove_table(QObject*)));
  emit changed();
}

void EnvClient::add_post()
{
  DescCache* desc = new DescCache(qPrintable(_source_edit->text()),
                                  _scalar_plot->title(),
                                  Ami::PostAnalysis);

  EnvPost* post = new EnvPost(*_filter->filter(),
                              desc,
                              _set);

  _posts.push_back(post);

  emit changed();
}

void EnvClient::remove_plot(QObject* obj)
{
  EnvPlot* plot = static_cast<EnvPlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  emit changed();
}

void EnvClient::remove_table(QObject* obj)
{
  EnvTable* tabl = static_cast<EnvTable*>(obj);
  _tabls.remove(tabl);

  disconnect(tabl, SIGNAL(destroyed(QObject*)), this, SLOT(remove_table(QObject*)));
  emit changed();
}


void EnvClient::select_source()
{
  FeatureCalculator* c = new FeatureCalculator(this, "Source", FeatureRegistry::instance(_set));
  if (c->exec()==QDialog::Accepted) {
    _source_edit->setText(c->result());
  }
  delete c;
}

void EnvClient::validate_source()
{
  //  Don't know how to validate yet
}

void EnvClient::plot(const QString& name, 
                     DescEntry*     desc, 
                     SharedData*    shared)
{
  EnvPlot* plot = new EnvPlot(this,
			      name,
			      Ami::RawFilter(),
			      desc,
                              _set,
                              shared);

  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  connect(plot, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));

  emit changed();
}

void EnvClient::add_overlay(DescEntry*  desc,
                            QtPlot*     plot,
                            SharedData* shared)
{
  EnvOverlay* ovl = new EnvOverlay(*this,
                                   *plot,
                                   *_filter->filter(),
                                   desc,
                                   _set,
                                   shared);
                                     
  _ovls.push_back(ovl);
  connect(ovl, SIGNAL(changed()), (AbsClient*)this, SIGNAL(changed()));
  
  emit changed();
}

void EnvClient::remove_overlay(QtOverlay* obj)
{
  EnvOverlay* ovl = static_cast<EnvOverlay*>(obj);
  _ovls.remove(ovl);

  //  emit changed();
}

