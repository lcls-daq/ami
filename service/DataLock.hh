#ifndef Ami_DataLock_hh
#define Ami_DataLock_hh

#include <list>

namespace Ami {
  class Semaphore;
  class DataLock {
  public:
    DataLock();
    ~DataLock();
  public:
    Semaphore* read_register();
    void  read_resign  (Semaphore*);
  public:
    void  write_lock();
    void  write_unlock();
  public:
    static void disable();
  private:
    std::list<Semaphore*> _locks;
  };
};

#endif
