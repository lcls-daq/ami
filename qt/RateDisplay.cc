#include "ami/qt/RateDisplay.hh"
#include "ami/qt/RateCalculator.hh"
#include "ami/qt/CpuCalculator.hh"
#include "ami/qt/RecvCalculator.hh"
#include "ami/qt/RunMaster.hh"
#include "ami/client/ClientManager.hh"
#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScan.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EnvPlot.hh"
#include "ami/data/RawFilter.hh"
#include "ami/service/Socket.hh"
#include "ami/service/Task.hh"
#include "ami/service/ConnectionManager.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

static const int BufferSize = 0x8000;

static const unsigned InputRateSignature  = 1;
static const unsigned AcceptRateSignature = 2;
static const unsigned RunNumberSignature  = 3;

using namespace Ami::Qt;

RateDisplay::RateDisplay(ConnectionManager& conn,
                         ClientManager* manager) :
  _manager         (manager),
  _input           (0),
  _description     (new char[BufferSize]),
  _cds             ("RateDisplay"),
  _niovload        (5),
  _iovload         (new iovec[_niovload]),
  _inputCalc       (new RateCalculator),
  _acceptCalc      (new RateCalculator),
  _cpuCalc         (new CpuCalculator),
  _netCalc         (new RecvCalculator(conn)),
  _task            (new Task(TaskObject("rattmr"))),
  _ready           (false)
{
  Timer::start();
}

RateDisplay::~RateDisplay()
{
  Timer::cancel();

  _cds.reset();

  delete _manager;
  delete _inputCalc;
  delete _acceptCalc;
  delete _cpuCalc;
  delete _netCalc;
  delete[] _iovload;
  delete[] _description;
}

void RateDisplay::expired()
{
  _netCalc->update();

  if (_ready)
    _manager->request_payload();

  _ready = false;
}

void RateDisplay::addLayout(QVBoxLayout* l)
{
  QGroupBox* rate_box  = new QGroupBox("Load");
  QHBoxLayout* layout = new QHBoxLayout;    
  layout->addWidget(new QLabel("Input:"));
  layout->addWidget(_inputCalc);
  layout->addStretch();
  layout->addWidget(new QLabel("Filtered:"));
  layout->addWidget(_acceptCalc);
  layout->addStretch();
  layout->addWidget(new QLabel("CPU:"));
  layout->addWidget(_cpuCalc);
  layout->addStretch();
  layout->addWidget(new QLabel("Network:"));
  layout->addWidget(_netCalc);
  layout->setMargin(0);
  layout->setSpacing(0);
  rate_box->setLayout(layout);
  l->addWidget(rate_box); 
}

int  RateDisplay::configure       (char*& p)
{ 
  RunMaster::instance()->reset();

  if (_input) {
    { Ami::EnvPlot op(Ami::DescScalar("ProcTime","mean"));
      ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                      ConfigureRequest::Discovery,
                                                      _input,
                                                      InputRateSignature,
                                                      RawFilter(), op);
      p += r.size(); }

    { Ami::EnvPlot op(Ami::DescScalar("ProcTimeAcc","mean"));
      ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                      ConfigureRequest::Discovery,
                                                      _input,
                                                      AcceptRateSignature,
                                                      RawFilter(), op);
      p += r.size(); }

    { Ami::EnvPlot op(Ami::DescScan("RunNumber","RunNumber","RunNumber",1));
      ConfigureRequest& r = *new (p) ConfigureRequest(ConfigureRequest::Create,
                                                      ConfigureRequest::Discovery,
                                                      _input,
                                                      RunNumberSignature,
                                                      RawFilter(), op);
      p += r.size(); }
  }
  return 1;
}

void RateDisplay::discovered      (const DiscoveryRx& rx) 
{ 
  const DescEntry* e = rx.entry(Ami::EntryScalar::input_entry());
  if (e)
    _input = e->signature();
  else {
    printf("RateDisplay failed to find input\n");
    _input = -1;

    rx.dump();
  }
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

  _cds.reset();

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

  _ready = 
    _inputCalc ->set_entry(_cds.entry(InputRateSignature)) &&
    _acceptCalc->set_entry(_cds.entry(AcceptRateSignature)) &&
    _cpuCalc   ->set_entry(_cds.entry(InputRateSignature));

  RunMaster::instance()->set_entry(_cds.entry(RunNumberSignature));
}

int RateDisplay::read_payload    (Socket& s,int) {
  return s.readv(_iovload,_cds.totalentries());
}

void RateDisplay::process         () {
  _inputCalc ->update();
  _acceptCalc->update();
  _cpuCalc   ->update();
  RunMaster::instance()->update();
  _ready = true;
}
 
