//
//  ServerManager manages the connection of a server to a peer client.
//  The peer's client manager requests a connection, and ServerManager
//  allocates a server to match that request.  If the request cannot
//  be fulfilled, the client manager is informed so it can make an
//  alternate request.
//
#include "ServerManager.hh"

#include "ami/data/Message.hh"
#include "ami/server/Server.hh"
#include "ami/server/VServerSocket.hh"
#include "ami/service/TSocket.hh"
#include "ami/service/Exception.hh"

#include <errno.h>
#include <string.h>

using namespace Ami;


ServerManager::ServerManager(unsigned interface,
			     unsigned serverGroup) :
  Poll        (1000),
  _interface  (interface),
  _serverGroup(serverGroup),
  _socket     (0)
{
}


ServerManager::~ServerManager()
{
}


void ServerManager::serve(Factory& factory)
{
  _factory = &factory;
  try {
    if (Ins::is_multicast(_serverGroup))
      _socket = new VServerSocket(Ins(_serverGroup,Port::serverPort()),
				  _interface);
    else {
      TSocket* so = new TSocket;
      so->bind(Ins(Port::serverPort()));
      ::listen(so->socket(),5);
      _socket = so;
    }
  } catch (Event& e) {
    printf("SM::serve %s : %s\n",e.who(),e.what());
    return;
  }
  manage(*this);
}

void ServerManager::dont_serve()
{
  unmanage(*this);
  delete _socket;
  _socket = 0;
}

void ServerManager::discover()
{
  Message msg(0,Message::DiscoverReq);
  bcast_in(reinterpret_cast<const char*>(&msg),sizeof(msg));
}

int ServerManager::fd() const { return _socket->socket(); }

int ServerManager::processIo()
{
  Message request(0,Message::NoOp);

  if (!(Ins::is_multicast(_serverGroup))) {
    Sockaddr addr;
    unsigned length = addr.sizeofName();
    int s = ::accept(_socket->socket(), addr.name(), &length);
    ::read(s, &request, sizeof(request));
    ::close(s);
  }
  else
    _socket->read(&request, sizeof(request));

  if (request.type()==Message::Connect) {
    try {
      TSocket* s = new TSocket;

      Ins remote(request.payload(),request.offset());
      s->connect(remote);
      Ins local (s->ins());

      printf("ServerManager::connect new ServerSocket %d bound to local: %x/%d  remote: %x/%d\n",
	     s->socket(), 
	     local .address(), local .portId(),
	     remote.address(), remote.portId());

      Server* srv = new Server(s,*_factory,request);
      _servers.push_back(srv);
      manage(*srv);
     } 
    catch (Event& e) {
      printf("Connect failed: %s\n",e.what());
    }
  }
  return 1;
}

int ServerManager::processTmo() { return 1; }
