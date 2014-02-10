#include "ami/python/L3TClient.hh"

#include "ami/client/ClientManager.hh"

#include "ami/data/ConfigureRequest.hh"

#include <semaphore.h>
#include <stdio.h>

//#define DBUG

using namespace Ami::Python;

static const int BufferSize = 0x8000;

//
//  
//
L3TClient::L3TClient(AbsFilter* f) :
  _filter          (f),
  _request         (new char[BufferSize]),
  _manager         (0)
{
  sem_init(&_initial_sem, 0, 0);
}

L3TClient::~L3TClient() 
{
  if (_manager) delete _manager;

  delete[] _request;

  sem_destroy(&_initial_sem);
}

void L3TClient::connected()
{
  if (_manager)
    _manager->discover();
}

void L3TClient::discovered(const DiscoveryRx& rx)
{
  _manager->configure();
}

int  L3TClient::configure       (iovec* iov) 
{
  char* p = _request;

  ConfigureRequest& r = 
    *new(p) ConfigureRequest(ConfigureRequest::Filter,
                             (1<<31),
                             *_filter);
  p += r.size();

  iov[0].iov_base = _request;
  iov[0].iov_len  = p - _request;
  return 1;
}

int  L3TClient::configured      () 
{
  return 0; 
}

int  L3TClient::read_description(Socket& socket,int len)
{
  sem_post(&_initial_sem);
  return 0;
}

int  L3TClient::read_payload     (Socket& socket,int)
{
  return 0;
}

bool L3TClient::svc             () const { return true; }

void L3TClient::process         () 
{
}

void L3TClient::managed(ClientManager& mgr)
{
  _manager = &mgr;
  _manager->connect();
}

int  L3TClient::initialize(ClientManager& mgr)
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

  return Success;
}
