//
//  Server handles all client requests for data description and payload
//
#include "Server.hh"

#include "ami/service/Socket.hh"
#include "ami/server/Factory.hh"
#include "ami/data/Discovery.hh"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <sstream>

//#define DBUG

using namespace Ami;

static const int BufferSize = 32*1024;
static const unsigned HistorySize = 5;

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

  for(unsigned i=0; i<HistorySize; i++)
    _history.push_back(_reply);
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

  _history.push_front(_reply);
  _history.pop_back();

#ifdef DBUG
  printf("S reply %d:%d %x  skt %d\n",
 	 _reply.id(), _reply.type(),_reply.payload(), _socket->socket());
#endif

  int nbytes = _socket->writev(_iov,cnt);
  if (nbytes < 0)
    printf("Error in Server::reply writev writing %d bytes : %s\n",_reply.payload(),strerror(errno));
  else if (unsigned(nbytes) != _reply.payload()+sizeof(_reply)) {
    printf("Error in Server::reply write wrote %d/%zd bytes\n",
           nbytes, _reply.payload()+sizeof(_reply));
  }
}

std::string Server::dump() const
{
  std::ostringstream s;
  std::list<Message>::const_iterator it=_history.begin();
  s << "Server : socket " << std::setw(2) << _socket->socket()
    << " : reply " << it->type_str()
    << "." << it->id() << std::endl;
  while(++it != _history.end())
    s << "                           " 
      << it->type_str()
      << "." << it->id() 
      << std::endl;
  return s.str();
}
