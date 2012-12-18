/*
 * semaphore routines implemented with solaris semapores
 * posix semaphores would be almost the same
 *
 */


#include "Semaphore.hh"

using namespace Ami;

Semaphore::Semaphore(semState initial)
{
  unsigned int count=0;

  if(initial == Semaphore::FULL) count = 1;

  sem_init(&_sem, 0, count);

}


Semaphore::~Semaphore()
{
  sem_destroy(&_sem);
}



void Semaphore::take()
{
  sem_wait(&_sem);
}

void Semaphore::give() 
{
  sem_post(&_sem);
}

bool Semaphore::take(unsigned dt_ms)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);

  tv.tv_sec += dt_ms/1000;
  unsigned dt_ns = (dt_ms%1000)*1000000 + tv.tv_nsec;
  if (dt_ns > 1000000000) {
    tv.tv_nsec = dt_ns-1000000000;
    tv.tv_sec++;
  }
  else {
    tv.tv_nsec = dt_ns;
  }

  return sem_timedwait(&_sem,&tv)==0;
}
