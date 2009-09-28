#include "EnvClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/FeatureRegistry.hh"
#include "ami/qt/EnvPlot.hh"
#include "ami/qt/DescTH1F.hh"
#include "ami/qt/DescChart.hh"
#include "ami/qt/DescProf.hh"

#include "ami/client/VClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescTH1F.hh"
#include "ami/data/DescScalar.hh"
#include "ami/data/DescProf.hh"
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

enum { _TH1F, _vT, _vF };

static const int BufferSize = 0x8000;

EnvClient::EnvClient(QWidget* parent) :
  QWidget(parent,::Qt::Window),
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

  _source = new QComboBox;
  _source->addItems(FeatureRegistry::instance().names());

  _features = new QComboBox;
  _features->addItems(FeatureRegistry::instance().names());

  _hist   = new DescTH1F  ("Sum (1dH)");
  _vTime  = new DescChart ("Sum v Time",0.2);
  _vFeature = new DescProf("Sum v Var",_features);

  _plot_grp = new QButtonGroup;
  _plot_grp->addButton(_hist    ->button(),_TH1F);
  _plot_grp->addButton(_vTime   ->button(),_vT);
  _plot_grp->addButton(_vFeature->button(),_vF);
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
  connect(&FeatureRegistry::instance(), SIGNAL(changed()), this, SLOT(change_features()));
  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
  connect(this, SIGNAL(changed()), this, SLOT(update_configuration()));
}

EnvClient::~EnvClient() {}

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

void EnvClient::read_description(Socket& socket)
{
  printf("Described so\n");
  int size = socket.read(_description,BufferSize);

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

void EnvClient::read_payload     (Socket& socket)
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

void EnvClient::managed(VClientManager& mgr)
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
  switch(_plot_grp->checkedId()) {
  case _TH1F:
    desc = new Ami::DescTH1F(qPrintable(_source->currentText()),
			     "value","events",
			     _hist->bins(),_hist->lo(),_hist->hi()); 
    break;
  case _vT: 
    desc = new Ami::DescScalar(qPrintable(_source->currentText()),
			       "mean");
    break;
  case _vF:
    desc = new Ami::DescProf(qPrintable(_source->currentText()),
			     qPrintable(_vFeature->variable()),"mean",
			     _vFeature->bins(),_vFeature->lo(),_vFeature->hi(),"mean");
    break;
  default:
    desc = 0;
    break;
  }

  EnvPlot* plot = new EnvPlot(_source->currentText(),
			      desc,
			      FeatureRegistry::instance().index(_source->currentText()),
			      FeatureRegistry::instance().index(_vFeature->variable()));
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

void EnvClient::change_features()
{
  _source->clear();
  _source->addItems(FeatureRegistry::instance().names());
  _source->setCurrentIndex(0);

  _features->clear();
  _features->addItems(FeatureRegistry::instance().names());
  _features->setCurrentIndex(0);
}
