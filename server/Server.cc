//
//  Server handles all client requests for data description and payload
//
#include "Server.hh"

#include "ami/service/Socket.hh"
#include "ami/server/Factory.hh"
#include "ami/data/Discovery.hh"

#include <errno.h>

//#define DBUG

using namespace Ami;

static const int BufferSize = 32*1024;

Server::Server(Socket*         socket) :
  _socket(socket),
  _iov   (new iovec[10]),
  _iovcnt(10),
  _reply (0,Message::NoOp),
  _buffer (new char[BufferSize]),
  _described(false)
{
  _socket->setsndbuf(0x400000);  // payloads can be quite large

  _iov[0].iov_base = &_reply;
  _iov[0].iov_len  = sizeof(_reply);
}

Server::~Server()
{
  delete _socket;
  delete[] _buffer;
  delete[] _iov;
}

int Server::fd() const { return _socket->socket(); }

void Server::_adjust    (int size)
{
  if (size > _iovcnt) {
    delete[] _iov;
    _iov = new iovec[size+1];
    _iov[0].iov_base = &_reply;
    _iov[0].iov_len  = sizeof(_reply);
    _iovcnt = size;
  }
}

void Server::reply(unsigned id, Message::Type type, unsigned cnt)
{
  _reply.id(id);
  _reply.type(type);
  _reply.payload(_iov+1,cnt-1);

#ifdef DBUG
  printf("S reply %d:%d %x  skt %d\n",
 	 _reply.id(), _reply.type(),_reply.payload(), _socket->socket());
#endif

  int nbytes = _socket->writev(_iov,cnt);
  if (nbytes < 0)
    printf("Error in Server::reply writev writing %d bytes : %s\n",_reply.payload(),strerror(errno));
  else if (unsigned(nbytes) != _reply.payload()+sizeof(_reply)) {
    printf("Error in Server::reply write wrote %d/%d bytes\n",
           nbytes, _reply.payload()+sizeof(_reply));
  }
}
