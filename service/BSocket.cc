#include "ami/service/BSocket.hh"

#include <sys/socket.h>
#include <string.h>

using namespace Ami;

BSocket::BSocket(unsigned size) :
  _size(size),
  _buffer(new char[size])
{
}

BSocket::~BSocket()
{
}

int BSocket::readv(const iovec* iov, int iovcnt)
{
  char* data = _buffer;
  while(iovcnt--) {
    memcpy(iov->iov_base, data, iov->iov_len);
    data += iov->iov_len;
    iov++;
  }
  return data-_buffer;
}

char* BSocket::data() { return _buffer; }
