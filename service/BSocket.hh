#ifndef Ami_BSocket_HH
#define Ami_BSocket_HH

/**
 **  class BSocket - an implementation of the Socket class which 
 **                  holds the data in an intermediate memory buffer.
 **/

#include "ami/service/Socket.hh"

#include <sys/types.h>

namespace Ami {

  class Message;

  class BSocket : public Socket {
  public:
    BSocket(unsigned size);
    ~BSocket();
  public:    
    ///  Read data from the memory buffer into the iovec array
    virtual int readv(const iovec* iov, int iovcnt);
  public:
    ///  Accessor to the beginning of the memory buffer
    char* data();
    ///  Size of the data currently held in the buffer
    unsigned size() const;
  private:
    unsigned _size;
    char*    _buffer;
  };
};

#endif
