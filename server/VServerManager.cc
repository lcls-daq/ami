//
//  VServerManager manages the connection of a server to a peer client.
//  The peer's client manager requests a connection, and VServerManager
//  allocates a server to match that request.  If the request cannot
//  be fulfilled, the client manager is informed so it can make an
//  alternate request.
//
#include "VServerManager.hh"

#include "ami/data/Message.hh"
#include "ami/server/VServer.hh"
#include "ami/server/VServerSocket.hh"
#include "ami/server/Exception.hh"

#include <errno.h>
#include <string.h>

using namespace Ami;


VServerManager::VServerManager(unsigned interface,
			       Factory& factory) :
  VPoll     (-1),
  _interface(interface),
  _factory  (factory),
  _socket   (0)
{
}


VServerManager::~VServerManager()
{
}


void VServerManager::serve()
{
  _socket = new VServerSocket(Ins(Port::allServers(),Port::serverPortBase()),
			      _interface,
			      Ins(Port::allClients(),Port::clientPort()));
  Message request(0, Message::Reconnect);
  _socket->write(&request, sizeof(request));
  manage(*this);
}

void VServerManager::dont_serve()
{
  unmanage(*this);
  delete _socket;
  _socket = 0;
}

void VServerManager::remove(VServer* srv)
{
  _servers.remove(srv);
  delete srv;
}

int VServerManager::fd() const { return _socket->socket(); }

int VServerManager::processIo()
{
  Message request(0,Message::NoOp);
  _socket->peek(&request);

  printf("request type %d id %d\n",request.type(), request.id());

  if (request.type()==Message::Connect) {
    try {
      VServerSocket* s = new VServerSocket(Ins(Port::allServers(),request.payload()),
					   _interface,
					   _socket->peer());
      VServer* srv = new VServer(s,*this,_factory);
      _servers.push_back(srv);
    } 
    catch (Event& e) {
      Message reply(request.id(),Message::Connect,-1);
      _socket->write(&reply,sizeof(reply));
    }
  }
  _socket->flush();
  return 1;
}

int VServerManager::processTmo() { return 1; }
