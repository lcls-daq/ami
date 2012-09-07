//
//  CollectionServer handles all client requests for data description and payload
//
#include "CollectionServer.hh"

#include "ami/service/BSocket.hh"
#include "ami/service/ConnectionManager.hh"
#include "ami/service/Poll.hh"
#include "ami/client/ClientManager.hh"
#include "ami/data/Discovery.hh"

#include <errno.h>

using namespace Ami;

static const int BufferSize = 32*1024;

CollectionServer::CollectionServer(unsigned           interface,
                                   unsigned           serverGroup,
                                   ConnectionManager& connect_mgr,
                                   Socket*            socket,
                                   bool               postService) :
  Server(socket),
  _client_manager(new ClientManager(interface,
                                    serverGroup,
                                    connect_mgr,
                                    *this)),
  _post_service(postService)
{
  _client_manager->connect();
}

CollectionServer::~CollectionServer()
{
  delete _client_manager;
}

int CollectionServer::processIo()
{
  int r = 1;
  Message request(0,Message::NoOp);
  int result = _socket->read(&request,sizeof(request));
  if (result != sizeof(request)) {
    printf("S processIo read result %d (skt %d): %s\n",
	   result,_socket->socket(),strerror(errno));
    return 0;
  }

#ifdef DBUG
  printf("CS processIo %d:%d %x\n", request.id(), request.type(), request.payload());
#endif

  switch(request.type()) {
  case Message::ConfigReq:
    _client_manager->forward(request, *_socket);
    break;
  case Message::Disconnect:
    r = 0;
  case Message::DiscoverReq:
  case Message::DescriptionReq:
  case Message::PayloadReq:
    _client_manager->forward(request);
  default:
    break;
  }

  return r;
}

int CollectionServer::processIo(const char* msg,int size)
{
  const Message& request = *reinterpret_cast<const Message*>(msg);

#ifdef DBUG
  printf("CS processIo %d:%d %x\n", request.id(), request.type(), request.payload());
#endif

  switch(request.type()) {
  case Message::DiscoverReq:
    _client_manager->forward(request);
    break;
  default:
    break;
  }
  return 1;
}

// AbsClient interface
void CollectionServer::connected       () {}
void CollectionServer::disconnected    () {}
int  CollectionServer::configure       (iovec* iov) { printf("CS configure ***\n"); abort(); return 1; }
int  CollectionServer::configured      () { printf("CS configured ***\n"); abort(); return 1; }
void CollectionServer::discovered      (const DiscoveryRx& rx)
{
#ifdef DBUG
  printf("CS _reply %d:%d %x\n", -1, Message::Discover, rx.payload_size());
#endif

  _adjust(2);
  _iov[1].iov_base = const_cast<char*>(rx.payload());
  _iov[1].iov_len  = rx.payload_size();
  reply(-1, Message::Discover, 2);
}
int  CollectionServer::read_description(Socket& s,int len)
{
  _reply(Message::Description, s, len);
  return len;
}
int  CollectionServer::read_payload    (Socket& s,int len)
{
  _reply(Message::Payload, s, len);
  return len;
}
bool CollectionServer::svc             () const
{
  return _post_service;
}

void CollectionServer::process         () {}

void CollectionServer::_reply(Message::Type t, Socket& s, int len)
{
#ifdef DBUG
  printf("CS _reply %d:%d %x\n", _client_manager->request().id(), t, len);
#endif

#if 1
  char* p = new char[len];
  s.read(p,len);

  _iov[1].iov_base = p;
  _iov[1].iov_len  = len;
  reply(_client_manager->request().id(), t, 2);

  delete[] p;
#else
  BSocket& b  = static_cast<BSocket&>(s);
  _iov[1].iov_base = b.data();
  _iov[1].iov_len  = len;
  reply(_client_manager->request().id(), t, 2);
#endif
}
