#ifndef Ami_ConnectionManager_hh
#define Ami_ConnectionManager_hh

/*************************************************
**  Handles connections to a well-known TCP port *
*************************************************/

#include "Ins.hh"
#include "Routine.hh"
#include "Semaphore.hh"

#include <list>

namespace Ami {

  class Task;
  class TSocket;
  class ConnectionHandler;

  class ConnectionManager : public Routine {
  public:
    ConnectionManager(int ipaddr);
    ~ConnectionManager();

    Ins        ins   () const;
    unsigned   add   (ConnectionHandler&);
    void       remove(ConnectionHandler&);

    void routine();

    unsigned   receive_bytes();
  private:
    TSocket*                      _socket;
    Task*                         _task;
    Semaphore                     _listen_sem;
    std::list<ConnectionHandler*> _handlers;
    unsigned                      _connect_id;
  };
};

#endif
