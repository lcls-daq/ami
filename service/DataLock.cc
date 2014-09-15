#include "ami/service/DataLock.hh"
#include "ami/service/Semaphore.hh"

static bool _use_lock=true;

using namespace Ami;

DataLock::DataLock() {}

DataLock::~DataLock() 
{
  for(std::list<Semaphore*>::iterator it=_locks.begin();
      it!=_locks.end(); it++)
    delete *it;
  _locks.clear();
}

Semaphore* DataLock::read_register()
{
  Semaphore* sem = new Semaphore(Semaphore::FULL);
  _locks.push_back(sem);
  return sem;
}

void DataLock::read_resign(Semaphore* sem)
{
  _locks.remove(sem);
  delete sem;
}

void DataLock::write_lock()
{
  if (_use_lock)
    for(std::list<Semaphore*>::iterator it=_locks.begin();
	it!=_locks.end(); it++)
      (*it)->take();
}

void DataLock::write_unlock()
{
  if (_use_lock)
    for(std::list<Semaphore*>::iterator it=_locks.begin();
	it!=_locks.end(); it++)
      (*it)->give();
}

void DataLock::disable() { _use_lock=false; }
