#include "EnvClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/FeatureBox.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/EnvPlot.hh"
#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"
#include "ami/qt/DescScan.hh"

#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/service/Socket.hh"
#include "ami/service/Semaphore.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>

#include <sys/types.h>
#include <sys/socket.h>

using namespace Ami::Qt;

enum { _TH1F, _vT, _vF, _vS };

static const int BufferSize = 0x8000;
static const Pds::DetInfo envInfo(0, Pds::DetInfo::NoDetector,0,Pds::DetInfo::Evr,0);

EnvClient::EnvClient(QWidget* parent) :
  QtTopWidget      (parent,envInfo,0),
  _title           ("Env"),
  _input           (0),
  _output_signature(0),
  _request         (new char[BufferSize]),
  _description     (new char[BufferSize]),
  _cds             ("Client"),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _sem             (new Semaphore(Semaphore::EMPTY))
{
  setWindowTitle(QString("Environment"));
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this);
  _status  = new Status;

  _source = new FeatureBox;

  _hist   = new DescTH1F  ("Sum (1dH)");
  _vTime  = new DescChart ("Mean v Time",0.2);
  _vFeature = new DescProf("Mean v Var" );
  _vScan    = new DescScan("Mean v Scan");

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_hist    ->button(),_TH1F);
  _plot_grp->addButton(_vTime   ->button(),_vT);
  _plot_grp->addButton(_vFeature->button(),_vF);
  _plot_grp->addButton(_vScan   ->button(),_vS);
  _hist->button()->setChecked(true);

  QPushButton* plotB  = new QPushButton("Plot");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_control);
    layout1->addStretch();
    layout1->addWidget(_status);
    layout->addLayout(layout1); }
  { QGroupBox* channel_box = new QGroupBox("Source Channel");
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_source);
    layout1->addStretch();
    channel_box->setLayout(layout1);
    layout->addWidget(channel_box); }
  { QGroupBox* plot_box = new QGroupBox("Plot");
    QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(_hist );
    layout1->addWidget(_vTime);
    layout1->addWidget(_vFeature);
    layout1->addWidget(_vScan);
    plot_box->setLayout(layout1); 
    layout->addWidget(plot_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(plotB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(plotB     , SIGNAL(clicked()),      this, SLOT(plot()));
  connect(closeB    , SIGNAL(clicked()),      this, SLOT(hide()));
  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect(this, SIGNAL(changed()), this, SLOT(update_configuration()));
}

EnvClient::~EnvClient() {}

const QString& EnvClient::title() const { return _title; }

void EnvClient::save(char*& p) const
{
  QtPWidget::save(p);

  _hist    ->save(p);
  _vTime   ->save(p);
  _vFeature->save(p);
  _source  ->save(p);

  QtPersistent::insert(p,_plot_grp->checkedId ());

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++) {
    QtPersistent::insert(p,QString("EnvPlot"));
    (*it)->save(p);
  }
  QtPersistent::insert(p,QString("EndEnvPlot"));

  _control->save(p);
}

void EnvClient::load(const char*& p)
{
  QtPWidget::load(p);

  _hist    ->load(p);
  _vTime   ->load(p);
  _vFeature->load(p);
  _source  ->load(p);

  _plot_grp->button(QtPersistent::extract_i(p))->setChecked(true);

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    disconnect(*it, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
  _plots.clear();

  QString name = QtPersistent::extract_s(p);
  while(name == QString("EnvPlot")) {
    EnvPlot* plot = new EnvPlot(this, p);
    _plots.push_back(plot);
    connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

    name = QtPersistent::extract_s(p);
  }

  _control->load(p);
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

void EnvClient::reset_plots() { update_configuration(); }

void EnvClient::connected()
{
  _status->set_state(Status::Connected);
  _manager->discover();
}

void EnvClient::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);
  printf("Discovered\n");

  const DescEntry* e = rx.entries();
  _input = e->signature();

  //  iterate through discovery and print
  FeatureRegistry::instance().clear ();
  FeatureRegistry::instance().insert(rx.feature_name(0),rx.features());

  _manager->configure();
}

int  EnvClient::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);
  printf("Configure\n");

  char* p = _request;

  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->configure(p,_input,_output_signature);

  if (p > _request+BufferSize) {
    printf("Client request overflow: size = 0x%x\n", p-_request);
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
  printf("Configured\n");
  return 0; 
}

void EnvClient::read_description(Socket& socket,int len)
{
  printf("Described so\n");
  int size = socket.read(_description,len);

  if (size<0) {
    printf("Read error in Ami::Qt::Client::read_description.\n");
    return;
  }

  if (size==BufferSize) {
    printf("Buffer overflow in Ami::Qt::Client::read_description.  Dying...\n");
    abort();
  }

  //  printf("emit description\n");
  emit description_changed(size);

  _sem->take();
}

void EnvClient::_read_description(int size)
{
  printf("Described si\n");

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

  _status->set_state(Status::Described);

  _sem->give();
}

void EnvClient::read_payload     (Socket& socket,int)
{
  //  printf("payload\n"); 
  if (_status->state() == Status::Requested) {
    socket.readv(_iovload,_cds.totalentries());
  }
  else {
    //
    //  Add together each server's contribution
    //
    printf("Ami::Qt::Client::read_payload: multiple server processing not implemented\n");
  }
  _status->set_state(Status::Received);
}

void EnvClient::process         () 
{
  //
  //  Perform client-side processing
  //
  for(std::list<EnvPlot*>::const_iterator it=_plots.begin(); it!=_plots.end(); it++)
    (*it)->update();
  
  _status->set_state(Status::Processed);
}

void EnvClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  printf("EnvClient connecting\n");
  _manager->connect();
}

void EnvClient::request_payload()
{
  if (_status->state() >= Status::Described) {
    _manager->request_payload();
    _status->set_state(Status::Requested);
  }
}

void EnvClient::update_configuration()
{
  _manager->configure();
}

void EnvClient::plot()
{
  DescEntry* desc;
  QString entry(_source->entry());

  switch(_plot_grp->checkedId()) {
  case _TH1F:
    desc = new Ami::DescTH1F(qPrintable(entry),
			     qPrintable(entry),"events",
			     _hist->bins(),_hist->lo(),_hist->hi()); 
    break;
  case _vT: 
    desc = new Ami::DescScalar(qPrintable(entry),"mean");
    break;
  case _vF:
    desc = new Ami::DescProf(qPrintable(entry),
			     qPrintable(_vFeature->expr()),"mean",
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean");
    break;
  case _vS:
    desc = new Ami::DescScan(qPrintable(entry),
			     qPrintable(_vScan->expr()),qPrintable(entry),
			     _vScan->bins());
    break;
  default:
    desc = 0;
    break;
  }

  EnvPlot* plot = new EnvPlot(this,
			      entry,
			      desc);

  _plots.push_back(plot);

  connect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));

  emit changed();
}

void EnvClient::remove_plot(QObject* obj)
{
  EnvPlot* plot = static_cast<EnvPlot*>(obj);
  _plots.remove(plot);

  disconnect(plot, SIGNAL(destroyed(QObject*)), this, SLOT(remove_plot(QObject*)));
}

