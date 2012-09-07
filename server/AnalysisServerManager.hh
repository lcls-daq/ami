#ifndef Ami_AnalysisServerManager_hh
#define Ami_AnalysisServerManager_hh

#include "ami/server/ServerManager.hh"

namespace Ami {

  class Factory;
  class Semaphore;
  class Socket;

  class AnalysisServerManager : public ServerManager {
  public:
    AnalysisServerManager (unsigned interface,
                           unsigned serverGroup);
    ~AnalysisServerManager();
  public:
    Server* new_server(Socket*, bool);
  public:
    void serve     (Factory&, Semaphore* =0);
  private:
    Factory*           _factory;
  };
};

#endif
