#ifndef Ami_ClientManager_hh
#define Ami_ClientManager_hh

#include "ami/data/Message.hh"
#include "ami/data/Aggregator.hh"
#include "ami/service/ConnectionHandler.hh"
#include "ami/service/Ins.hh"
#include "ami/service/DumpSource.hh"
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

  /**
   *   A class for managing the client end of a connection with the set of
   *   monitoring server processes.  This object listens 
   */
  class ClientManager : public ConnectionHandler,
			public DumpSource {
  public:
    /**
     *  Constructor which listens on a local interface for server processes
     *  that belong to the indicated group.  Connections to those servers
     *  are managed by a TCP connection manager.  The client object which
     *  consumes the plottable data is also passed.
     *
     *  param[in]  interface          IP address of local interface
     *  param[in]  serverGroup        IP multicast address of server group
     *  param[in]  connect_mgr        TCP connection manager
     *  param[in]  client             Data consumer
     */
    ClientManager(unsigned   interface,
		  unsigned   serverGroup,
		  ConnectionManager& connect_mgr,
		  AbsClient& client);
    ~ClientManager();
  public:
    ///  Request connection to the servers in the group
    void connect   ();
    ///  Disconnect from all servers in the group
    void disconnect();
    ///  Request discovery of data sources in the stream
    void discover  ();
    ///  Configure analyses to be performed by the servers
    void configure ();
    ///  Request description of analyses output data shapes
    void request_description();
    ///  Request update of output data payload
    ///    Argument is true for "push-mode" / continuous updates
    void request_payload    (bool=false);
    ///  Request update of a sparsified set of output data payload
    void request_payload    (const EntryList& request);
    ///  Request termination of continuous updates
    void request_stop       ();
  public:
    ///  Return id of connection assigned by ConnectionManager
    virtual unsigned connection_id() const;
    ///  Register a new server connection by its network socket
    virtual void     handle(int socket);
    ///  Return count of received data bytes since last call
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
  public:
    void dump_throttle() const;
  public:
    std::string dump() const;
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
    
