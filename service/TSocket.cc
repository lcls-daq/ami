#include "TSocket.hh"

#include "ami/data/Message.hh"
#include "ami/service/Sockaddr.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

//#define POLL_RCV
//#define DBUG

using namespace Ami;

TSocket::TSocket() throw(Event)
{
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw Event("TSocket failed to open socket",strerror(errno));

  int parm = 16*1024*1024;
  if(setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1) {
    printf("TSocket failed to set sndbuf: %s\n",strerror(errno));
    throw Event("TSocket failed to set sndbuf",strerror(errno));
  }

  if(setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1) {
    printf("TSocket failed to set rcvbuf: %s\n",strerror(errno));
    throw Event("TSocket failed to set sndbuf",strerror(errno));
  }

  _rhdr.msg_name       = 0;
  _rhdr.msg_namelen    = 0;
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;
  _rhdr.msg_flags      = 0;
}

TSocket::TSocket(int s) throw(Event)
{
  int parm = 16*1024*1024;
  if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1) {
    printf("TSocket failed to set sndbuf: %s\n",strerror(errno));
    throw Event("TSocket failed to set sndbuf",strerror(errno));
  }

  if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1) {
    printf("TSocket failed to set rcvbuf: %s\n",strerror(errno));
    throw Event("TSocket failed to set sndbuf",strerror(errno));
  }

  _rhdr.msg_name       = 0;
  _rhdr.msg_namelen    = 0;
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;
  _rhdr.msg_flags      = 0;

  _socket = s;
}

//
//  Bind to port to listen
//
//void TSocket::bind(const Ins& insb) throw(Event)
//
//  I find that exceptions are not caught (in C++) when
//  used as a module in python.  Revert to checking return value.
//
bool TSocket::bind(const Ins& insb)
{
  _rhdr.msg_control    = 0;
  _rhdr.msg_controllen = 0;

  int optval=1;
  if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    throw Event("TSocket failed to set reuseaddr",strerror(errno));

  Sockaddr sa(insb);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0) {
    //    throw Event("TSocket failed to bind to port",strerror(errno));
    return false;
  }
  return true;
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
  Sockaddr sa(peer);
  if (::connect(_socket, sa.name(), sa.sizeofName())<0)
    throw Event("TSocket failed to connect to peer",strerror(errno));
}

TSocket::~TSocket()
{
}

int TSocket::readv(const iovec* iov, int iovcnt)
{
#ifdef POLL_RCV
  //
  //  Try to handle incomplete reads without hanging
  //
  iovec* _iov = new iovec[iovcnt];
  int remaining = 0;
  for(int i=0; i<iovcnt; i++) {
    remaining += iov[i].iov_len;
  }

  int bytes = 0;

  while(remaining) {

    int _iovcnt = 0;
    for(int i=0, b=bytes; i<iovcnt; i++) {
      if (b < iov[i].iov_len) {
        _iov[_iovcnt].iov_base = (char*)iov[i].iov_base+b;
        _iov[_iovcnt].iov_len  = iov[i].iov_len-b;
        b = 0;
        _iovcnt++;
      }
      else {
        b -= iov[i].iov_len;
      }
    }      

    _rhdr.msg_iov    = const_cast<iovec*>(_iov);
    _rhdr.msg_iovlen = _iovcnt;

    int nb = ::recvmsg(_socket, &_rhdr, 0);
    if (nb < 0) break;
    bytes += nb;
    remaining -= nb;
  }    
    
#else
  _rhdr.msg_iov    = const_cast<iovec*>(iov);
  _rhdr.msg_iovlen = iovcnt;

  //  A read of 0 bytes + MSG_WAITALL = hang
  if (iovcnt==0 || iov[0].iov_len==0) return 0;

  int bytes = ::recvmsg(_socket, &_rhdr, MSG_WAITALL);
#endif

  if (bytes<0) {
    //    printf("Error reading from skt %d : %s\n",
    //	   socket(), strerror(errno));
    //    abort();
  }
  return bytes;
}

