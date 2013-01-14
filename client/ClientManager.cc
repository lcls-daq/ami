//
//  ClientManager manages the connection and disconnection of a client
//  to a server in the peer group.  Since there can be many servers 
//  processing data in parallel, all requests will be answered with 
//  multiple replies.  The last reply should be followed by a timeout.
//  Connection requests are sent to the ServerManagers of the peer group.
//  All other requests go to the allocated Servers.
//
#include "ClientManager.hh"

#include "ami/service/Sockaddr.hh"
#include "ami/service/Ins.hh"
#include "ami/service/Port.hh"
#include "ami/service/Poll.hh"
#include "ami/service/Task.hh"
#include "ami/service/ConnectionManager.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/Message.hh"
#include "ami/data/Aggregator.hh"

#include "ami/client/AbsClient.hh"
#include "ami/client/ClientSocket.hh"
#include "ami/client/VClientSocket.hh"

#include "ami/server/VServerSocket.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netpacket/packet.h>

//#define DBUG

namespace Ami {
  //
  //  A class to listen for servers that come online
  //
  class ServerConnect : public Routine {
  public:
    ServerConnect(ClientManager& mgr,
		  Socket*        skt) :
      _task(new Task(TaskObject("cmco"))),
      _mgr(mgr), _skt(skt), _found(false) 
    {
      _task->call(this); 
    }
    ~ServerConnect() {
      _task->destroy_b();
      delete _skt;
    }
  public:
    void routine()
    {
      pollfd fds[1];
      fds[0].fd = _skt->socket();
      fds[0].events = POLLIN | POLLERR;

      if (poll(fds, 1, 1000)>0) {
        Message msg(0,Message::NoOp);
        if (_skt->read(&msg,sizeof(msg))==sizeof(msg))
          if (msg.type()==Message::Hello) {
#ifdef DBUG
	    printf("Hello from socket %d\n", _skt->socket());
#endif
            _found = true;
          }
      }
      else if (_found) {
        _found = false;
        _mgr.connect();
      }
      _task->call(this);
    }
  private:
    Task*          _task;
    ClientManager& _mgr;
    Socket*        _skt;
    bool           _found;
  };

  class ProxyConnect : public Routine {
  public:
    ProxyConnect(ClientManager& mgr,
                 Socket*&       skt,
                 const Ins&     ins) :
      _task(new Task(TaskObject("cmco"))),
      _mgr(mgr), _skt(skt), _ins(ins), _found(false) 
    {
      _task->call(this); 
    }
    ~ProxyConnect() {
      _task->destroy_b();
      delete _skt;
    }
  public:
    void routine()
    {
      if (_skt==0) {
        TSocket* skt = new TSocket;
        try {
          skt->connect(_ins);
          _skt = skt;
#ifdef DBUG
	  printf("Proxy online\n");
#endif
        }
        catch(Event& e) {
#ifdef DBUG
	  printf("ProxyConnect failed\n");
#endif
          delete skt;
	  sleep(1);
        }
      }
      else {
        pollfd fds[1];
        fds[0].fd = _skt->socket();
        fds[0].events = POLLIN | POLLERR;

        if (poll(fds, 1, 1000)>0) {
          Message msg(0,Message::NoOp);
          int len = _skt->read(&msg,sizeof(msg));
          if (len==sizeof(msg)) {
            if (msg.type()==Message::Hello) {
#ifdef DBUG
              printf("Hello from socket %d\n",
                     _skt->socket());
#endif
              _found = true;
            }
          }
          else if (len<=0) {
#ifdef DBUG
            printf("Proxy offline\n");
#endif
            delete _skt;
            _skt = 0;
          }
        }
        else if (_found) {
          _found = false;
          _mgr.connect();
        }
      }
      _task->call(this);
    }
  private:
    Task*          _task;
    ClientManager& _mgr;
    Socket*&       _skt;
    Ins            _ins;
    bool           _found;
  };

  class ClPoll : public Poll {
  public:
    ClPoll(ClientManager& cm) : Poll(1000), _cm(cm) {}
  public:
    int processTmo() { _cm.processTmo(); return 1; }
  private:
    ClientManager& _cm;
  };

};

using namespace Ami;

static const int Step = 20;
static const int Timeout = 20;  // milliseconds
static const int BufferSize = 0x8000;

static void dump(const char* payload, unsigned size) __attribute__((unused));

static void dump(const char* payload, unsigned size)
{
  const unsigned* p = (const unsigned*)payload;
  const unsigned* const e = p+(size>>2);
  for(unsigned k=0; p < e; k++,p++)
      printf("%08x%c", *p, (k%8)==7 ? '\n' : ' ');
  printf("\n");
}


ClientManager::ClientManager(unsigned   interface,
			     unsigned   serverGroup,
			     ConnectionManager& connect_mgr,
			     AbsClient& client) :
  _client     (client),
  _poll       (new ClPoll(*this)),
  _state      (Disconnected),
  _request    (0,Message::NoOp),
  _iovs       (new iovec[Step]),
  _buffer_size(BufferSize),
  _buffer     (new char[BufferSize]),
  _discovery_size(BufferSize),
  _discovery  (new char[BufferSize]),
  _client_sem (Semaphore::EMPTY),
  _server     (serverGroup, Port::serverPort()),
  _connect_mgr(connect_mgr),
  _connect_id (connect_mgr.add(*this)),
  _receive_bytes(0),
  _receive_last (0)
{
  bool mcast = Ins::is_multicast(serverGroup);

#ifdef DBUG
  printf("CM int %x grp %x mcast %c poll %p\n", 
	 interface, serverGroup,
	 mcast ? 'T':'F',
         _poll);
#endif

  if (mcast) {
    VClientSocket* so = new VClientSocket;
    so->set_dst(_server, interface);
    _connect = so;
    _reconn = new ServerConnect(*this, new VServerSocket(_server, interface));
  }
  else {
    TSocket* so = new TSocket;
    _connect = 0;
    try {
      so->connect(_server);
      _connect = so;
    }
    catch(Event& e) {
      delete so;
    }
    _reconn = new ProxyConnect(*this, _connect, _server);
  }

  _poll->start();
}


ClientManager::~ClientManager()
{
  _connect_mgr.remove(*this);
  disconnect();
  delete[] _iovs;
  delete[] _buffer;
  delete[] _discovery;
  _poll->stop();
  delete _poll;
  if (_reconn ) delete _reconn ;
  //  delete &_client;
}

void ClientManager::request_payload()
{
  request_payload(EntryList(EntryList::Full));
}

void ClientManager::request_payload(const EntryList& req)
{
  if (_state == Connected) {
    _client.request_payload(req);
    _request = Message(_request.id()+1,Message::PayloadReq,req);
    _poll->bcast_out(reinterpret_cast<const char*>(&_request),
		     sizeof(_request));
  }
}

//
//  Connecting involves negotiating a destination port with the server group
//  and allocating a server on the peer connected to that port.  
//
void ClientManager::connect()
{
  //  Remove previous connections
  unsigned n = nconnected();
  while(n)
    delete &_poll->fds(n--);

  if (_connect) {
    _request = Message((_client.svc() ? (1<<31):0) | _connect_id,
                       Message::Connect,
                       _connect_mgr.ins().address(),
                       _connect_mgr.ins().portId());
    _connect->write(&_request,sizeof(_request));
  }
}

int ClientManager::nconnected() const { return _poll->nfds(); }

//
//  Disconnecting closes the server on the peer
//
void ClientManager::disconnect()
{
  if (_state != Disconnected && nconnected()) {
    _request = Message(_request.id()+1,Message::Disconnect);
    _poll->bcast_out(reinterpret_cast<const char*>(&_request),
		     sizeof(_request));
    _state = Disconnected;
  }
}

void ClientManager::discover()
{
  _request = Message(_request.id()+1,Message::DiscoverReq);
  _poll->bcast_out(reinterpret_cast<const char*>(&_request),
		   sizeof(_request));
}

//
//  Build the configuration request from the client.  A description request
//  is implied.
//
void ClientManager::configure()
{
  int n = _client.configure(_iovs+1);
  _request = Message(_request.id()+1,Message::ConfigReq);
  _request.payload(_iovs+1,n);
  _iovs[0].iov_base = &_request;
  _iovs[0].iov_len  = sizeof(_request);
  _poll->bcast_out(_iovs, n+1);
  _client.configured();
}

unsigned ClientManager::connection_id() const
{
  return _connect_id;
}

void ClientManager::handle(int s)
{
  new ClientSocket(*this,s);
  _state = Connected;
}

unsigned ClientManager::receive_bytes()
{
  unsigned rxb = _receive_bytes - _receive_last;
  _receive_last = _receive_bytes;
  return rxb;
}

void ClientManager::add_client(ClientSocket& socket) 
{
  _client.connected();
  _poll->manage(socket);
}

void ClientManager::remove_client(ClientSocket& socket)
{
  _poll->unmanage(socket);
  _client.disconnected();
  _client_sem.give();
}

void ClientManager::_flush_sockets(const Message& reply,
				   ClientSocket& socket)
{
  for(int i=0; i<nconnected(); i++) {
    ClientSocket& s = static_cast<ClientSocket&>(_poll->fds(i));
    if (&s != &socket) {
      Message r(0,Message::NoOp);
      s.read(&r,sizeof(r));
      if (r.type() != reply.type()) {
	printf("_flush_sockets type %d from socket %d\n",
	       r.type(), s.fd());
      }
      _flush_socket(s,r.payload());
    }
  }
}

void ClientManager::_flush_socket(ClientSocket& socket,
                                  int           remaining)
{
  while(remaining) {
    int sz = socket.read(_buffer,remaining < BufferSize ? 
                         remaining : BufferSize);
    remaining -= sz;
  } 
}

int ClientManager::handle_client_io(ClientSocket& socket)
{
  Message reply(0,Message::NoOp);
  if (socket.read(&reply,sizeof(reply))!=sizeof(reply)) {
    return 0;
  }

#ifdef DBUG
  printf("CM handle_client_io %d:%d %x\n",
         reply.id(), reply.type(), reply.payload());
#endif

  int size = 0;

  if (reply.type() == Message::Discover) { // unsolicited message
    if (_discovery_size < reply.payload()) {
      delete[] _discovery;
      _discovery = new char[_discovery_size=reply.payload()];
    }
    size = socket.read(_discovery,reply.payload());
    //    dump(_discovery,size);
    DiscoveryRx rx(_discovery, size);
    _client.discovered(rx, reply.id());
  }
  else if (reply.id() == _request.id()) {   // solicited message
  //  else if (1) { // solicited message
    switch (reply.type()) {
    case Message::Description: 
      size = _client.read_description(socket, reply.payload(), reply.id());
      break;
    case Message::Payload:     
      size =_client.read_payload(socket,reply.payload(), reply.id());
      _receive_bytes += size;
      _client.process();
      break;
    default:          
      break;
    }
  }
  else {
    //
    //  Sink the unsolicited message
    //
#ifdef DBUG
    printf("(%p) received id %d/%d type %d/%d\n",
  	   this, reply.id(),_request.id(),reply.type(),_request.type());
#endif
    
    switch (reply.type()) {
    case Message::Description: 
      size = _client.read_description(socket, reply.payload(), reply.id());
      break;
    default:
      break;
    }
  }

  if (size < int(reply.payload())) {
    switch (reply.type()) {
    case Message::Discover: 
    case Message::Description: 
    case Message::Payload:     
      _flush_socket(socket,reply.payload()-size);
      break;
    default:          
      break;
    }
  }
  return 1;
}

void ClientManager::forward(const Message& request)
{
  if (request.type() == Message::PayloadReq)
    _client.request_payload(request.list());

  _request = request;
  _poll->bcast_out(reinterpret_cast<const char*>(&request),
                   sizeof(request));
#ifdef DBUG
  printf("Forward %d:%d %x to %d clients\n",
         request.id(), request.type(), 0,_poll->nfds());
#endif
}

void ClientManager::forward(const Message& request,
                            Socket&        socket)
{
  _request = request;

  unsigned len = request.payload();
  char* p = new char[len];
  socket.read(p, len);

  iovec iov[2];
  iov[0].iov_base = const_cast<Message*>(&request);
  iov[0].iov_len  = sizeof(request);
  iov[1].iov_base = p;
  iov[1].iov_len  = len;

  _poll->bcast_out(iov,2);

#ifdef DBUG
  printf("Forward %d:%d %x to %d clients\n",
         request.id(), request.type(),
         len,_poll->nfds());
#endif

  delete[] p;
}

int ClientManager::processTmo()
{
  _client.tmo();
  return 1;
}
