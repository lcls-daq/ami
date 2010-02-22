#include "TSocket.hh"

#include "ami/data/Message.hh"
#include "ami/service/Sockaddr.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace Ami;

TSocket::TSocket() throw(Event) :
  _iovs(new iovec[_iovcnt=10])
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw Event("TSocket failed to open socket",strerror(errno));

  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;
}

TSocket::TSocket(int s) throw(Event) :
  _iovs(new iovec[_iovcnt=10])
{
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;

  _socket = s;
}

//
//  Bind to port to listen
//
void TSocket::bind(const Ins& insb) throw(Event)
{
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;

  int optval=1;
  if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    throw Event("TSocket failed to set reuseaddr",strerror(errno));

  printf("Binding to %x/%d\n",insb.address(),insb.portId());
  Sockaddr sa(insb);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0)
    throw Event("TSocket failed to bind to port",strerror(errno));

  Ins src = ins();
  printf("TSocket bound to %x/%d\n",src.address(),src.portId());
}

Ins TSocket::ins() const throw(Event)
{
  Sockaddr name;
  socklen_t len = name.sizeofName();
  if (::getsockname(_socket, name.name(), &len) < 0)
    throw Event("TSocket failed to lookup name",strerror(errno));
  return name.get();
}

//
//  Connects to peer
//
void TSocket::connect(const Ins& peer) throw(Event) 
{
  printf("Connecting to %x/%d\n",peer.address(),peer.portId());
  Sockaddr sa(peer);
  if (::connect(_socket, sa.name(), sa.sizeofName())<0)
    throw Event("TSocket failed to connect to peer",strerror(errno));
}

TSocket::~TSocket()
{
}

int TSocket::readv(const iovec* iov, int iovcnt)
{
  int bytes = 0;
  _rhdr.msg_iov    = const_cast<iovec*>(iov);
  _rhdr.msg_iovlen = iovcnt;
  bytes += ::recvmsg(_socket, &_rhdr, 0);
  return bytes;
}

