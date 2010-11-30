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

#include "ami/data/Cds.hh"
#include "ami/data/Discovery.hh"
#include "ami/data/Message.hh"
#include "ami/data/Aggregator.hh"

#include "ami/client/AbsClient.hh"
#include "ami/client/ClientSocket.hh"
#include "ami/client/VClientSocket.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <string.h>
#include <stdio.h>

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


ClientManager::ClientManager(unsigned   ppinterface,
			     unsigned   interface,
			     unsigned   serverGroup,
			     unsigned short port,
			     AbsClient& client) :
  _client    (*new Aggregator(client)),
  _ppinterface(ppinterface),
  _port       (port),
  _poll      (new Poll(1000)),
  _state     (Disconnected),
  _request   (0,Message::NoOp),
  _iovs      (new iovec[Step]),
  _buffer    (new char[BufferSize]),
  _discovery (new char[BufferSize]),
  _task      (new Task(TaskObject("lstn"))),
  _listen_sem(Semaphore::EMPTY),
  _client_sem(Semaphore::EMPTY),
  _server    (serverGroup, Port::serverPort())
{
  if (Ins::is_multicast(serverGroup)) {
    VClientSocket* so = new VClientSocket;
    so->set_dst(_server, interface);
    _connect = so;

    _listen = new TSocket;
    try             { _listen->bind(Ins(ppinterface,port)); }
    catch(Event& e) { printf("bind error : %s\n",e.what()); }

    _task->call(this);
    _listen_sem.take();
  }
  else {
    _connect = 0;
    _listen  = 0;
  }

  _poll->start();
}


ClientManager::~ClientManager()
{
  disconnect();
  delete[] _iovs;
  delete[] _buffer;
  delete[] _discovery;
  _task->destroy();
  if (_listen ) delete _listen ;
  if (_connect) delete _connect;
}

void ClientManager::request_payload()
{
  if (_state == Connected) {
    _request = Message(_request.id()+1,Message::PayloadReq);
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
  if (_connect) {
    _request = Message(_request.id()+1,Message::Connect,
                       _ppinterface,
                       _listen->ins().portId());
    printf("(%p) CM Request connection from %x/%d\n",
           this,_ppinterface,_listen->ins().portId());
    _connect->write(&_request,sizeof(_request));
  }
  else {
    ClientSocket& cs = *new ClientSocket(*this);
    try              {
      cs.bind   (Ins(_ppinterface,_port));
      Ins remote = _server;
      cs.connect(remote); 
      Ins local  = cs.ins();
      printf("(%p) new ClientSocket %d bound to local: %x/%d  remote: %x/%d\n",
             this, cs.fd(), 
             local .address(), local .portId(),
             remote.address(), remote.portId());
      _state = Connected;
      _client.connected();
    }
    catch (Event& e) { printf("Connection failed: %s\n", e.what()); delete &cs; }
  }
}

int ClientManager::nconnected() const { return _poll->nfds()-1; }

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

void ClientManager::routine()
{
  while(1) {
    if (::listen(_listen->socket(),5)<0)
      printf("ClientManager listen failed\n");
    else {
      _listen_sem.give();
      Ami::Sockaddr name;
      unsigned length = name.sizeofName();
      int s = ::accept(_listen->socket(),name.name(), &length);
      if (s<0)
	printf("ClientManager accept failed\n");
      else {
	ClientSocket* cs = new ClientSocket(*this,s);
	Ins local  = cs->ins();
	Ins remote = name.get();
	printf("(%p) new ClientSocket %d bound to local: %x/%d  remote: %x/%d\n",
	       this, s, 
	       local .address(), local .portId(),
	       remote.address(), remote.portId());
	_state = Connected;
	_client.connected();
      }
    }
  }
}

void ClientManager::add_client(ClientSocket& socket) 
{
  printf("(%p) ClientManager::add_client %d (skt %d)\n",
	 this, nconnected(),socket.socket());
  _poll->manage(socket);
}

void ClientManager::remove_client(ClientSocket& socket)
{
  _poll->unmanage(socket);
  printf("(%p) ClientManager::remove_client %d\n",
	 this,nconnected());
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
      s.read(_buffer,r.payload());
    }
  }
}

int ClientManager::handle_client_io(ClientSocket& socket)
{
  Message reply(0,Message::NoOp);
  if (socket.read(&reply,sizeof(reply))!=sizeof(reply)) {
    printf("Error reading from skt %d : %s\n",
	   socket.socket(), strerror(errno));
    return 0;
  }

//   printf("(%p) handle_client_io type %d from socket %d\n",
// 	 this, reply.type(), socket.fd());

  if (reply.type() == Message::Discover) { // unsolicited message
    int size = socket.read(_discovery,reply.payload());
    //    dump(_discovery,size);
    DiscoveryRx rx(_discovery, size);
    _client.discovered(rx);
  }
  else if (reply.id() == _request.id()) {   // solicited message
  //  else if (1) { // solicited message
    switch (reply.type()) {
    case Message::Description: 
      _client.read_description(socket, reply.payload());
      //      _request = Message(_request.id()+1,_request.type());  // only need one reply
      break;
    case Message::Payload:     
      _client.read_payload(socket,reply.payload());
      _client.process();
      break;
    default:          
      break;
    }
  }
  else {
    printf("(%p) received id %d/%d type %d/%d\n",
	   this, reply.id(),_request.id(),reply.type(),_request.type());
    switch (reply.type()) {
    case Message::Description: 
    case Message::Payload:     
      { int remaining = reply.payload();
	while(remaining) {
	  int sz = socket.read(_buffer,remaining < BufferSize ? 
			       remaining : BufferSize);
	  remaining -= sz;
	} 
      }
      break;
    default:          
      break;
    }
  }
  return 1;
}
