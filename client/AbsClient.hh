#ifndef Ami_AbsClient_hh
#define Ami_AbsClient_hh

class iovec;

namespace Ami {
  class DiscoveryRx;
  class Message;
  class Socket;
  class AbsClient {
  public:
    virtual ~AbsClient() {}

    virtual void connected       () = 0;
    virtual int  configure       (iovec*) = 0;
    virtual int  configured      () = 0;
    virtual void discovered      (const DiscoveryRx&) = 0;
    virtual void read_description(Socket&) = 0;
    virtual void read_payload    (Socket&) = 0;
    virtual void process         () = 0;
  };
};

#endif
