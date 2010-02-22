#ifndef Ami_ClientManager_hh
#define Ami_ClientManager_hh

#include "ami/data/Message.hh"
#include "ami/service/Routine.hh"

#include <list>
using std::list;

class iovec;

namespace Ami {

  class AbsClient;
  class ClientSocket;
  class VClientSocket;
  class Poll;
  class Task;
  class TSocket;

  class ClientManager : public Routine {
  public:
    ClientManager(unsigned   interface,
		  unsigned   serverGroup,
		  unsigned short port,
		  AbsClient& client);
    ~ClientManager();
  public:
    void connect   ();  // Connect to a server group
    void disconnect();  // Disconnect from a server group
    void discover  ();
    void configure ();
    void request_description();
    void request_payload    ();
  public:    // Routine interface
    virtual void routine();
  public:
    void add_client      (ClientSocket&);
    void remove_client   (ClientSocket&);
    void handle_client_io(ClientSocket&);
  private:
    int  _nconnected() const;
    void _flush_sockets(const Message&, ClientSocket&);
  private:
    enum State { Disconnected, Connected };
    AbsClient&      _client;
    Poll*           _poll;
    State           _state;
    Message         _request;
    iovec*          _iovs;
    char*           _buffer;
    char*           _discovery;

    Task*           _task;
    TSocket*        _listen;
    VClientSocket*  _connect;
  };

};

#endif
    
