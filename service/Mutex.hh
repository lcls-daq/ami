#ifndef PDS_MUTEX_HH
#define PDS_MUTEX_HH

#include <pthread.h>

namespace Ami {
  class Mutex {
  public:
    Mutex(const char* name);
    ~Mutex();
    void lock();
    void unlock();
  private:
    const char* _name;
    pthread_t _owner;
    pthread_cond_t _condition;
    pthread_mutex_t _mutex;
  };
}

#endif
