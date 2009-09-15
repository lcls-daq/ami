//
//  Server handles all client requests for data description and payload
//
#include "Server.hh"

#include "ami/server/VServerSocket.hh"
#include "ami/server/VServerManager.hh"
#include "ami/server/Factory.hh"

using namespace Ami;

static const int BufferSize = 32*1024;

Server::Server(VServerSocket*  socket,
	       VServerManager& mgr,
	       Factory&        factory) :
  VPoll  (-1),
  _socket(socket),
  _mgr   (mgr),
  _iov   (new iovec[10]),
  _iovcnt(10),
  _reply (0,Message::NoOp),
  _cds   ("Cds"),
  _factory(factory),
  _buffer (new char[BufferSize])
{
  _iov[0].iov_base = &_reply;
  _iov[0].iov_len  = sizeof(_reply);

  manage(*this);
  
  // factory appends discovery information
  const Cds& disc = _factory.discovery();
  int n = disc.description() + 1;
  _adjust( n );
  disc.description(_iov+1);
  reply(request.id(), Message::Connect, n);
}

Server::~Server()
{
  unmanage(*this);
  delete _socket;
  delete[] _buffer;
}

int Server::processTmo() { return 1; }

int Server::fd() const { return _socket->socket(); }

int Server::processIo()
{
  Message request(0,Message::NoOp);
  _socket->peek(&request);

  switch(request.type()) {
  case Message::Disconnect:
    _mgr.remove(this);
    break;
  case Message::ConfigReq:
    _socket->read(_buffer,BufferSize);
    _factory.configure(request,_buffer,_cds);
  case Message::DescriptionReq:
    { int n = _cds.description()+1;
      _adjust(n);
      _cds.description(_iov+1);
      reply(request.id(), Message::Description, n); }
    break;
  case Message::PayloadReq:
    { int n = _cds.payload()+1;
      _adjust(n);
      _cds.payload(_iov+1);
      reply(request.id(), Message::Payload, n); }
    break;
  default:
    break;
  }
  _socket->flush();
  return 1;
}

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
  _reply.type(type);
  _reply.payload(_iov+1,cnt-1);
  _socket->writev(_iov,cnt);
}
