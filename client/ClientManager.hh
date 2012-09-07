#ifndef Ami_ClientManager_hh
#define Ami_ClientManager_hh

#include "ami/data/Message.hh"
#include "ami/data/Aggregator.hh"
#include "ami/service/ConnectionHandler.hh"
#include "ami/service/Ins.hh"
#include "ami/service/Semaphore.hh"

#include <list>
using std::list;

class iovec;

namespace Ami {

  class AbsClient;
  class ConnectionManager;
  class ClientSocket;
  class VClientSocket;
  class Poll;
  class Routine;
  class Socket;

  class ClientManager : public ConnectionHandler {
  public:
    ClientManager(unsigned   interface,
		  unsigned   serverGroup,
		  ConnectionManager& connect_mgr,
		  AbsClient& client);
    ~ClientManager();
  public:
    void connect   ();  // Connect to a server group
    void disconnect();  // Disconnect from a server group
    void discover  ();
    void configure ();
    void request_description();
    void request_payload    ();
    void request_payload    (const EntryList& request);
  public:    // ConnectionHandler interface
    virtual unsigned connection_id() const;
    virtual void     handle(int);
    virtual unsigned receive_bytes();
  public:
    void add_client      (ClientSocket&);
    void remove_client   (ClientSocket&);
    int  handle_client_io(ClientSocket&);
    int  nconnected      () const;
  public:
    const Message& request() const { return _request; }
    void forward(const Message&);
    void forward(const Message&,Socket&);
  public:
    int  processTmo();
  private:
    void _flush_sockets(const Message&, ClientSocket&);
    void _flush_socket (ClientSocket&, int);
  private:
    enum State { Disconnected, Connected };
    Aggregator      _client;
    Poll*           _poll;
    State           _state;
    Message         _request;
    iovec*          _iovs;
    unsigned        _buffer_size;
    char*           _buffer;
    unsigned        _discovery_size;
    char*           _discovery;
    
    Socket*         _connect;
    Semaphore       _client_sem;
    Ins             _server;
    Routine*        _reconn;

    bool            _svc;

    ConnectionManager& _connect_mgr;
    unsigned           _connect_id;
    unsigned           _receive_bytes;
    unsigned           _receive_last;
  };

};

#endif
    
