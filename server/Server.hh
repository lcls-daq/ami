#ifndef Ami_Server
#define Ami_Server

#include "ami/service/Fd.hh"

#include "ami/data/Message.hh"

class iovec;

namespace Ami {

  class Socket;

  class Server : public Fd {
  public:
    Server(Socket*);
    ~Server();
  public:  // Fd interface
    int fd() const;
  protected:
    void _adjust     (int);
    void reply       (unsigned,Message::Type,unsigned);
  protected:
    Socket*         _socket;
    iovec*          _iov;
    int             _iovcnt;
    Message         _reply;
    char*           _buffer;
    bool            _described;
  };

};

#endif
