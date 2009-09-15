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
#include "ami/server/Exception.hh"

#include <errno.h>
#include <string.h>

using namespace Ami;


ServerManager::ServerManager(unsigned interface,
			     unsigned serverGroup,
			     unsigned clientGroup) :
  Poll      (-1),
  _interface(interface),
  _serverGroup(serverGroup),
  _clientGroup(clientGroup),
  _socket   (0)
{
}


ServerManager::~ServerManager()
{
}


void ServerManager::serve(Factory& factory)
{
  _factory = &factory,
  _socket = new VServerSocket(Ins(_serverGroup,Port::serverPortBase()),
			      _interface,
			      Ins(_clientGroup,Port::clientPort()));
  Message request(0, Message::Reconnect);
  _socket->write(&request, sizeof(request));
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
  bcast(reinterpret_cast<const char*>(&msg),sizeof(msg));
}

int ServerManager::fd() const { return _socket->socket(); }

int ServerManager::processIo()
{
  Message request(0,Message::NoOp);
  _socket->peek(&request);

  Ins ins(_socket->peer().get());
//   printf("request type %d id %d from %x/%d\n",
// 	 request.type(), request.id(),
// 	 ins.address(),ins.portId());

  if (request.type()==Message::Connect) {
    try {
      VServerSocket* s = new VServerSocket(Ins(_serverGroup,request.payload()),
					   _interface,
					   _socket->peer());
      Server* srv = new Server(s,*_factory,request);
      _servers.push_back(srv);
      manage(*srv);
    } 
    catch (Event& e) {
      printf("Connect failed\n");
      Message reply(request.id(),Message::Connect,0);
      _socket->write(&reply,sizeof(reply));
    }
  }
  _socket->flush();
  return 1;
}

int ServerManager::processTmo() { return 1; }
