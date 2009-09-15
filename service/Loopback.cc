#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "Loopback.hh"
#include "ami/service/Ins.hh"
#include "ami/service/Sockaddr.hh"

using namespace Ami;

static const int LoopbackAddr = 0x7f000001;

Loopback::Loopback()
{
  if ((_socket = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return;
  }
  Ins loopback(LoopbackAddr);
  Sockaddr sa(loopback);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0) {
    close();
    return;
  }
  socklen_t addrlen = sizeof(sockaddr_in);
  sockaddr_in name;
  if (::getsockname(_socket, (sockaddr*)&name, &addrlen) < 0) {
    close();
    return;
  }
  if (::connect(_socket, (const sockaddr*)&name, addrlen) < 0) {
    close();
    return;
  }
}

Loopback::~Loopback()
{
}

int Loopback::readv(const iovec* iov, int iovcnt)
{
  _hdr.msg_iov    = const_cast<iovec*>(iov);
  _hdr.msg_iovlen = iovcnt;
  return ::recvmsg(_socket, &_hdr, 0);
}
