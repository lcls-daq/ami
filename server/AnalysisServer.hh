#ifndef Ami_AnalysisServer
#define Ami_AnalysisServer

#include "ami/server/Server.hh"
#include "ami/data/Cds.hh"

namespace Ami {

  class Socket;
  class Factory;

  class AnalysisServer : public Server {
  public:
    AnalysisServer(Socket*,
                   Factory&);
    ~AnalysisServer();
  public:
    int processIo();
    int processIo(const char*,int);
  private:
    Cds             _cds;
    Factory&        _factory;
  };

};

#endif
