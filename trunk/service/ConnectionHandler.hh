#ifndef Ami_ConnectionHandler_hh
#define Ami_ConnectionHandler_hh

/**
 **  class ConnectionHandler - a class used by ConnectionManager for
 **                            establishing a TCP connection.
 **/

namespace Ami {

  class ConnectionHandler {
  public:
    virtual ~ConnectionHandler() {}

    ///  Return id of connection assigned by ConnectionManager
    virtual unsigned connection_id() const = 0;
    ///  Register a new server connection by its network socket
    virtual void     handle(int socket) = 0;
    ///  Return count of received data bytes since last call
    virtual unsigned receive_bytes() = 0;
  };
};

#endif
