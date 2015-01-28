#ifndef Ami_AnalysisServer
#define Ami_AnalysisServer

#include "ami/server/Server.hh"
#include "ami/service/Routine.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Message.hh"

namespace Ami {

  class Socket;
  class Factory;
  class EventFd;

  class AnalysisServer : public Server,
                         public Routine {
  public:
    AnalysisServer(Socket*,
                   Factory&,
                   EventFd*&,
                   bool post_svc);
    ~AnalysisServer();
  public:
    int  processIo();
    int  processIo(const char*,int);
    void routine();
  private:
    Cds             _cds;
    Factory&        _factory;
    EventFd*&       _event;
    Message         _repeat;
    bool            _post_service;
  };

};

#endif
