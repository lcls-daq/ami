#include "SummaryClient.hh"

#include "ami/qt/Control.hh"
#include "ami/qt/Status.hh"
#include "ami/qt/QtTH1F.hh"
#include "ami/qt/QtProf.hh"
#include "ami/qt/QtChart.hh"
#include "ami/qt/QtScan.hh"
#include "ami/qt/QtPlot.hh"

#include "ami/client/ClientManager.hh"

#include "ami/data/AbsTransform.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryTH1F.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryProf.hh"
#include "ami/data/EntryScan.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/RawFilter.hh"

#include "ami/service/Socket.hh"
#include "ami/service/Semaphore.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>

#include <sys/types.h>
#include <sys/socket.h>

namespace Ami {
  namespace Qt {
    class NullTransform : public Ami::AbsTransform {
    public:
      ~NullTransform() {}
      double operator()(double x) const { return x; }
    };

    class PagePlot : public QtPlot {
    public:
      PagePlot(QtBase* plot) : QtPlot(NULL,plot->title()), _plot(plot) { plot->attach(_frame); }
      ~PagePlot() { delete _plot; }
    public:
      void update() { _plot->update(); emit redraw(); }
      void dump(FILE*) const {}
    private:
      QtBase* _plot;
    };
  };
};

using namespace Ami::Qt;

static const int BufferSize = 0x8000;
static NullTransform noTransform;


SummaryClient::SummaryClient(QWidget* parent, const Pds::DetInfo& info, unsigned channel,
			     const QString& title, ConfigureRequest::Source source) :
  AbsClient        (parent, info, channel),
  _title           (title),
  _source          (source),
  _request         (new char[BufferSize]),
  _description     (new char[BufferSize]),
  _cds             ("Client"),
  _manager         (0),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _sem             (new Semaphore(Semaphore::EMPTY))
{
  setWindowTitle(title);
  setAttribute(::Qt::WA_DeleteOnClose, false);

  _control = new Control(*this);
  _status  = new Status;

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(_control);
    layout1->addStretch();
    layout1->addWidget(_status);
    layout->addLayout(layout1); }
  layout->addWidget(_tab = new QTabWidget);
  setLayout(layout);

  connect(this, SIGNAL(description_changed(int)), this, SLOT(_read_description(int)));
}

SummaryClient::~SummaryClient() {}

void SummaryClient::save(char*& p) const
{
  QtPWidget::save(p);
  _control->save(p);
}

void SummaryClient::load(const char*& p)
{
  QtPWidget::load(p);
  _control->load(p);
}

void SummaryClient::connected()
{
  _status->set_state(Status::Connected);
  _manager->discover();
}

void SummaryClient::discovered(const DiscoveryRx& rx)
{
  _status->set_state(Status::Discovered);
  printf("Summary Discovered\n");

  _manager->configure();
}

int  SummaryClient::configure       (iovec* iov) 
{
  _status->set_state(Status::Configured);
  printf("Summary Configure\n");

  char* p = _request;
  ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create, _source);
  p += r.size();

  iov[0].iov_base = _request;
  iov[0].iov_len  = p - _request;
  return 1;
}

int  SummaryClient::configured      () 
{
  printf("Summary Configured\n");
  return 0; 
}

void SummaryClient::read_description(Socket& socket,int len)
{
  printf("Summary Described so\n");
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

void SummaryClient::_read_description(int size)
{
  printf("Summary Described si\n");

  while(_tab->count()) {
    QWidget* w = _tab->widget(0);
    _tab->removeTab(0);
    delete w;
  }
  _cds.reset();

  const char* payload = _description;
  const char* const end = payload + size;

  payload += sizeof(Desc);

  while( payload < end ) {
    const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
    if (desc->size()==0) {
      printf("read_description size==0\n");
      break;
    }
    Entry* entry = EntryFactory::entry(*desc);
    printf("Summary found entry %s of type %d\n",
	   desc->name(), desc->type());
    QtBase* plot;
    QString title(desc->name());
    switch(desc->type()) {
    case Ami::DescEntry::TH1F: 
      plot = new QtTH1F(title,*static_cast<const Ami::EntryTH1F*>(entry),
			noTransform,noTransform,QColor(0,0,0));
      break;
    case Ami::DescEntry::Scalar:  // create a chart from a scalar
      plot = new QtChart(title,*static_cast<const Ami::EntryScalar*>(entry),
			 400,QColor(0,0,0));
      break;
    case Ami::DescEntry::Prof: 
      plot = new QtProf(title,*static_cast<const Ami::EntryProf*>(entry),
			noTransform,noTransform,QColor(0,0,0));
      break;
    case Ami::DescEntry::Scan: 
      plot = new QtScan(title,*static_cast<const Ami::EntryScan*>(entry),
			noTransform,noTransform,QColor(0,0,0));
      break;
    default:
      printf("SummaryClient type %d not implemented yet\n",desc->type()); 
      return;
    }
    _tab->addTab(new PagePlot(plot),title);
    _cds.add(entry, desc->signature());
    payload += desc->size();
  }
  if (_cds.totalentries()>_niovload) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=_cds.totalentries()];
  }
  _cds.payload(_iovload);

  _status->set_state(Status::Described);

  _sem->give();
}

void SummaryClient::read_payload     (Socket& socket,int)
{
  if (_status->state() == Status::Requested) {
    socket.readv(_iovload,_cds.totalentries());
  }
  else {
    printf("Ami::Qt::Client::read_payload unrequested payload\n");
  }
  _status->set_state(Status::Received);
}

void SummaryClient::process         () 
{
  QWidget* w = _tab->currentWidget();
  if (w)
    static_cast<PagePlot*>(w)->update();

  _status->set_state(Status::Processed);
}

void SummaryClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  show();
  printf("SummaryClient connecting\n");
  _manager->connect();
}

void SummaryClient::request_payload()
{
  if (_status->state() >= Status::Described) {
    _manager->request_payload();
    _status->set_state(Status::Requested);
  }
}

const QString& SummaryClient::title() const { return _title; }

void SummaryClient::save_plots(const QString&) const {}
void SummaryClient::reset_plots() {}