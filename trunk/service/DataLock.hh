#ifndef Ami_DataLock_hh
#define Ami_DataLock_hh

/**
 **  class DataLock - serializes critical access to data for
 **                   multiple readers, one writer
 **/

#include <list>

namespace Ami {
  class Semaphore;
  class DataLock {
  public:
    DataLock();
    ~DataLock();
  public:
    ///  Request a semaphore for protecting read access
    Semaphore* read_register();
    ///  Cancel protected access
    void  read_resign  (Semaphore*);
  public:
    ///  Lock access for writing
    void  write_lock();
    ///  Cancel write access lock
    void  write_unlock();
  public:
    ///  Globally cancel serialization
    static void disable();
  private:
    std::list<Semaphore*> _locks;
  };
};

#endif
