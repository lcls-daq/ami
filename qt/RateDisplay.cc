#include "ami/qt/RateDisplay.hh"
#include "ami/client/ClientManager.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EnvPlot.hh"
#include "ami/data/RawFilter.hh"
#include "ami/service/Socket.hh"
#include "ami/service/Task.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

static const int BufferSize = 0x8000;

static const unsigned InputRateSignature  = 1;
static const unsigned AcceptRateSignature = 2;

using namespace Ami::Qt;

class RateDisplay::RateCalculator {
public:
  RateCalculator() : _entry(0), _last(0), _display(new QLabel("0")) {}
  ~RateCalculator() {}
public:
  QLabel* display() { return _display; }

  void set_entry(Ami::Entry* entry) {
    _entry   = static_cast<Ami::EntryScalar*>(entry);
    _entries = _sum = _last = 0;
  }
  void update() { 
    if (_entry->valid()) {
      double n =  _entry->entries() - _entries;
      if (n > 0) {
        double v = n/(_entry->sum() - _sum);
        _entries = _entry->entries();
        _sum     = _entry->sum();
        _last    = v;
      }
      _display->setText(QString::number(_last,'f',1));
    }
    else
      _display->setText(QString("."));
  }
private:
  Ami::EntryScalar* _entry;
  double   _last;
  double   _entries;
  double   _sum;
  QLabel*  _display;
};

RateDisplay::RateDisplay(ClientManager* manager) :
  _manager         (manager),
  _input           (0),
  _description     (new char[BufferSize]),
  _cds             ("RateDisplay"),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _inputCalc       (new RateCalculator),
  _acceptCalc      (new RateCalculator),
  _task            (new Task(TaskObject("rattmr")))
{
  Timer::start();
}

RateDisplay::~RateDisplay()
{
  Timer::cancel();

  delete _inputCalc;
  delete _acceptCalc;
}

void RateDisplay::expired()
{
  _manager->request_payload();
}

void RateDisplay::addLayout(QVBoxLayout* l)
{
  QGroupBox* rate_box  = new QGroupBox("Rates");
  QHBoxLayout* layout = new QHBoxLayout;    
  layout->addWidget(new QLabel("Input:"));
  layout->addWidget(_inputCalc->display());
  layout->addStretch();
  layout->addWidget(new QLabel("Filtered:"));
  layout->addWidget(_acceptCalc->display());
  rate_box->setLayout(layout);
  l->addWidget(rate_box); 
}

int  RateDisplay::configure       (char*& p)
{ 
  { Ami::EnvPlot op(Ami::DescScalar("ProcPeriod","mean",""));
    ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Discovery,
                                                    _input,
                                                    InputRateSignature,
                                                    RawFilter(), op);
    p += r.size(); }

  { Ami::EnvPlot op(Ami::DescScalar("ProcPeriodAcc","mean",""));
    ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                    ConfigureRequest::Discovery,
                                                    _input,
                                                    AcceptRateSignature,
                                                    RawFilter(), op);
    p += r.size(); }

  return 1;
}

void RateDisplay::discovered      (const DiscoveryRx& rx) 
{ 
  printf("RD discovered payload %p  size %u\n",rx.payload(),rx.payload_size());
  _input = rx.entries()->signature();
}

void RateDisplay::read_description(Socket& s,int len) {

  int size = s.read(_description,len);

  if (size<0) {
    printf("Read error in Ami::Qt::RateDisplay::read_description.\n");
    return;
  }

  if (size==BufferSize) {
    printf("Buffer overflow in Ami::Qt::RateDisplay::read_description.  Dying...\n");
    abort();
  }

  const char* payload = _description;
  const char* const end = payload + size;
  payload += sizeof(Desc);
  while( payload < end ) {
    const DescEntry* desc = reinterpret_cast<const DescEntry*>(payload);
    Entry* entry = EntryFactory::entry(*desc);
    _cds.add(entry, desc->signature());
    payload += desc->size();
  }

  if (_cds.totalentries()>_niovload) {
    delete[] _iovload;
    _iovload = new iovec[_niovload=_cds.totalentries()];
  }
  _cds.payload(_iovload);

  _inputCalc ->set_entry(_cds.entry(InputRateSignature));
  _acceptCalc->set_entry(_cds.entry(AcceptRateSignature));
}

void RateDisplay::read_payload    (Socket& s,int) {
  s.readv(_iovload,_cds.totalentries());
}

void RateDisplay::process         () {
  _inputCalc ->update();
  _acceptCalc->update();
}
