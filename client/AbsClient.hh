#ifndef Ami_AbsClient_hh
#define Ami_AbsClient_hh

class iovec;

namespace Ami {
  class ClientManager;
  class DiscoveryRx;
  class Message;
  class Socket;
  /**
   *   A class for consuming the plottable data returned by the analysis servers.
   */
  class AbsClient {
  public:
    virtual ~AbsClient() {}

    ///  A ClientManager has been assigned to this client object
    virtual void managed         (ClientManager&) {}
    ///  The client has been connected to the analysis servers
    virtual void connected       () = 0;
    ///  The client has been disconnected
    virtual void disconnected    () {}
    /**
     *   The client should construct its configuration request on its own memory
     *   cache and assign the iovecs array to refer to it.  The number of iovec
     *   elements assigned is returned.
     */
    virtual int  configure       (iovec*) = 0;
    ///  The configuration was sent to the servers
    virtual int  configured      () = 0;
    ///  The data stream discovery has completed and its result is passed here to the client
    virtual void discovered      (const DiscoveryRx&) = 0;
    ///  The plottable data description of size bytes is ready to be read from socket.
    virtual void beginRun(unsigned) {}
    virtual void endRun  (unsigned) {}
    virtual int  read_description(Socket& socket,int size) = 0;
    ///  The plottable data payload of size bytes is ready to be read from socket.
    virtual int  read_payload    (Socket& socket,int size) = 0;
    ///  Process the data payload
    virtual void process         () = 0;
    ///  Processing should be done in post analysis stage
    virtual bool svc             () const = 0;
  };
};

#endif
