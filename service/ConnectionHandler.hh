#ifndef Ami_ConnectionHandler_hh
#define Ami_ConnectionHandler_hh

namespace Ami {

  class ConnectionHandler {
  public:
    virtual ~ConnectionHandler() {}

    virtual unsigned connection_id() const = 0;

    virtual void handle(int skt) = 0;
    virtual unsigned receive_bytes() = 0;
  };
};

#endif
