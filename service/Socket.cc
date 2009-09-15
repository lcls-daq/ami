#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "Socket.hh"
#include "ami/service/Ins.hh"

using namespace Ami;

Socket::Socket(int socket) : 
  _socket(socket)
{
  _hdr.msg_name       = 0;
  _hdr.msg_namelen    = 0;
  _hdr.msg_control    = 0;
  _hdr.msg_controllen = 0;
}

Socket::~Socket()
{
  if (_socket > 0) ::close(_socket);
}

int Socket::socket() const {return _socket;}

int Socket::read(void* buffer, int size)
{
  iovec iov;
  iov.iov_base = buffer;
  iov.iov_len  = size;
  return readv(&iov,1);
}

int Socket::write(const void* data, int size)
{
  iovec iov;
  iov.iov_base = const_cast<void*>(data);
  iov.iov_len  = size;
  return writev(&iov,1);
}

#include "pds/service/Sockaddr.hh"

int Socket::writev(const iovec* iov, int iovcnt)
{
  _hdr.msg_iov          = const_cast<iovec*>(iov);
  _hdr.msg_iovlen       = iovcnt;
  return ::sendmsg(_socket, &_hdr, MSG_DONTROUTE);
}

int Socket::setsndbuf(unsigned size) 
{
  if (::setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, 
		   (char*)&size, sizeof(size)) < 0) {
    return -1;
  }
  return 0;
}

int Socket::setrcvbuf(unsigned size) 
{
  if (::setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, 
		   (char*)&size, sizeof(size)) < 0) {
    return -1;
  }
  return 0;
}

int Socket::getsndbuf()
{
  int size;
  socklen_t ssize = sizeof(size);
  if (::getsockopt(_socket, SOL_SOCKET, SO_SNDBUF, 
		   (char*)&size, &ssize) < 0) {
    return -1;
  }
  return size;
}

int Socket::getrcvbuf()
{
  int size;
  socklen_t ssize = sizeof(size);
  if (::getsockopt(_socket, SOL_SOCKET, SO_RCVBUF, 
		   (char*)&size, &ssize) < 0) {
    return -1;
  }
  return size;
}

int Socket::close()
{
  if (::close(_socket) < 0) {
    return -1;
  } else {
    _socket = -1;
  }
  return 0;
}
