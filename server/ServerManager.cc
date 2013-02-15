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
#include "ami/service/Semaphore.hh"
#include "ami/service/TSocket.hh"
#include "ami/service/Exception.hh"
#include "ami/service/Port.hh"

#include "ami/client/VClientSocket.hh"

#include <errno.h>
#include <string.h>

using namespace Ami;


ServerManager::ServerManager(unsigned interface,
			     unsigned serverGroup) :
  Poll        (1000),
  _interface  (interface),
  _serverGroup(serverGroup),
  _socket     (0),
  _connect_sem(0)
{
}


ServerManager::~ServerManager()
{
}


void ServerManager::serve(Semaphore* sem)
{
  _connect_sem = sem;
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
  
  if (Ins::is_multicast(_serverGroup)) {
    VClientSocket s;
    s.set_dst(Ins(_serverGroup,Port::serverPort()),_interface);
    Message msg(0,Message::Hello);
    s.write(&msg,sizeof(msg));
    printf("Hello %x.%d\n",_serverGroup,Port::serverPort());
  }
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

void ServerManager::discover_post()
{
  Message msg(0,Message::DiscoverReq);
  post(reinterpret_cast<const char*>(&msg),sizeof(msg));
}

void ServerManager::register_key(unsigned key, int fd)
{
  int n = nfds()+1;
  for(int i=1; i<n; i++) {
    Fd& ifd = fds(i);
    if ((&ifd) && ifd.fd()==fd) {
      if (key >= _key_servers.size())
	_key_servers.resize(key+1);
      FdList& l = _key_servers[key];
      for(FdList::const_iterator it=l.begin(); it!=l.end(); it++)
	if (&ifd == *it) return;
      l.push_back(&ifd);
      break;
    }
  }
}

void ServerManager::discover_key(unsigned key)
{
  if (key < _key_servers.size()) {
    Message msg(0,Message::DiscoverReq);
    FdList& l = _key_servers[key];
    for(FdList::const_iterator it=l.begin(); it!=l.end(); it++) {
      (*it)->processIo(reinterpret_cast<const char*>(&msg),sizeof(msg));
    }
  }
}

void ServerManager::unmanage(Fd& fd)
{
  Server* srv = reinterpret_cast<Server*>(&fd);

  for(unsigned key=0; key<_key_servers.size(); key++)
    _key_servers[key].remove(srv);

  _servers.remove(srv);

  Poll::unmanage(fd);
}

int ServerManager::fd() const { return _socket->socket(); }

int ServerManager::processIo()
{
  Message request(0,Message::NoOp);

  if (!(Ins::is_multicast(_serverGroup))) {
    Sockaddr addr;
    unsigned length = addr.sizeofName();
    int fd = ::accept(_socket->socket(), addr.name(), &length);
    TSocket* s = new TSocket(fd);

    Ins local (s->ins());
    Ins remote = addr.get();

    printf("ServerManager::connect new ServerSocket %d bound to local: %x/%d  remote: %x/%d\n",
           s->socket(), 
           local .address(), local .portId(),
           remote.address(), remote.portId());

    s->read(&request, sizeof(request));

    if (request.type()!=Message::Connect) {
      printf("ServerManager::processIo expected Connect received %d\n",
             request.type());
      return 1;
    }

    Server* srv = new_server(s, request.post_service());
    if (request.post_service())
      _servers.push_back(srv);

    manage(*srv);

    if (_connect_sem) {
      _connect_sem->give();
      _connect_sem = 0;
    }
  }
  else {
    _socket->read(&request, sizeof(request));

    if (request.type()!=Message::Connect) {
      printf("ServerManager::processIo expected Connect received %d\n",
             request.type());
      return 1;
    }

    TSocket* s = new TSocket;
    try {
      Ins remote(request.payload(),request.offset());

      s->connect(remote);
      Ins local (s->ins());

      printf("ServerManager::connect new ServerSocket %d bound to local: %x/%d  remote: %x/%d\n",
	     s->socket(), 
	     local .address(), local .portId(),
	     remote.address(), remote.portId());

      Server* srv = new_server(s,request.post_service());
      if (request.post_service())
        _servers.push_back(srv);

      manage(*srv);

      uint32_t connect_id = request.id();
      ::write(s->socket(),&connect_id,sizeof(connect_id));

      if (_connect_sem) {
        _connect_sem->give();
        _connect_sem = 0;
      }
     } 
    catch (Event& e) {
      printf("Connect failed: %s\n",e.what());
      delete s;
    }
  }
  return 1;
}

int ServerManager::processTmo() { return 1; }

//
//  Command to special set of servers
//
int ServerManager::processIn (const char* payload, int size) 
{
  for(SvList::iterator it=_servers.begin(); it!=_servers.end(); it++)
    (*it)->processIo(payload,size);
  return 1;
}
