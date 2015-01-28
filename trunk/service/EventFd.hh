#ifndef Ami_SyncFd_hh
#define Ami_SyncFd_hh

#include "ami/service/Fd.hh"

#include <list>

namespace Ami {
  class Routine;
  class EventFd : public Fd {
  public:
    void insert(Routine& r) { _list.push_back(&r); }
    void remove(Routine& r) { _list.remove(&r); }
  protected:
    std::list<Routine*> _list;
  };
};

#endif
