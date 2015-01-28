#include "ami/python/Handler.hh"

#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/DescEntry.hh"
#include "ami/data/EntryFactory.hh"
#include "ami/data/EntryScalar.hh"

#include "ami/service/Socket.hh"

#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

//#define DBUG

using namespace Ami::Python;

static const int BufferSize = 0x8000;

//
//  Need all info to identify source and configure plot, filter
//
Handler::Handler(const Pds::DetInfo& info, 
		 unsigned options) :
  _info            (info),
  _options         (options),
  _request         (new char[BufferSize]),
  _manager         (0),
  _niovload        (5),
  _iovload         (new iovec[_niovload])
{
  sem_init(&_initial_sem, 0, 0);
}

Handler::~Handler() 
{
  if (_manager) delete _manager;

  delete[] _iovload;
  delete[] _request;

  sem_destroy(&_initial_sem);
}

void Handler::connected()
{
  if (_manager) {
    _manager->discover();
  }
}

void Handler::discovered(const DiscoveryRx& rx)
{
  _input = 0;
  for(  const DescEntry* e = rx.entries(); e < rx.end(); 
	e = reinterpret_cast<const DescEntry*>
	  (reinterpret_cast<const char*>(e) + e->size())) {
        
#ifdef DBUG
    printf("Handler Discovered info(%08x/%08x), channel(%d)\n",
	   e->info().log(),e->info().phy(), e->channel());
#endif
    
    if (e->info().level()==_info.level() &&
	e->info().phy  ()==_info.phy()) {
      _input    = e->signature();
      if (_options==-1U)
	_options  = e->options();
      break;
    }
  }
#ifdef DBUG
  printf("[%p] Handler Discovered (%d) [%08x]\n",this,_input,_info.phy());
#endif
  
  _manager->configure();
}

int  Handler::configure       (iovec* iov) 
{
  sem_post(&_initial_sem);
  if (_options != -1U) {
    char* p = _request;

    ConfigureRequest& r = *new (p) ConfigureRequest(_input,_options);
    p += r.size();
    iov[0].iov_base = _request;
    iov[0].iov_len  = p - _request;
    return 1;
  }
  return 0;
}

int  Handler::configured      () 
{
  return 0; 
}

int  Handler::read_description(Socket& socket,int len)
{
  return 0;
}

int  Handler::read_payload(Socket& socket,int) { return 0; }

bool Handler::svc             () const { return true; }

void Handler::process         () {}

void Handler::managed(ClientManager& mgr)
{
  _manager = &mgr;
  _manager->connect();
}

int  Handler::initialize(ClientManager& mgr)
{
  managed(mgr);

#ifdef DBUG
  timespec tmo;
  clock_gettime(CLOCK_REALTIME, &tmo);

  timespec btmo;
  btmo.tv_sec  = tmo.tv_sec;
  btmo.tv_nsec = tmo.tv_nsec;

  tmo.tv_sec += 5;
  int result = sem_timedwait(&_initial_sem, &tmo);

  clock_gettime(CLOCK_REALTIME, &tmo);
  printf("Ami::Client initialize time %f seconds\n",
         double(tmo.tv_sec-btmo.tv_sec) +
         (double(tmo.tv_nsec)-double(btmo.tv_nsec))*1.e-9);
#else
  timespec tmo;
  clock_gettime(CLOCK_REALTIME, &tmo);
  tmo.tv_sec+=5;
  int result = sem_timedwait(&_initial_sem, &tmo);
#endif

  if (result<0)
    return TimedOut;
  if (_input==0)
    return NoEntry;

  return Success;
}

