#ifndef AmiPy_Discovery_hh
#define AmiPy_Discovery_hh

#include "ami/client/AbsClient.hh"
#include "ami/service/Semaphore.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <list>

namespace Ami {

  class ClientManager;
  class ConnectionManager;
  class DiscoveryRx;

  namespace Python {
    class Discovery : public Ami::AbsClient {
    public:
      Discovery(unsigned ppinterface,
		unsigned interface,
		unsigned serverGroup);
      ~Discovery();
    public:
      Ami::ClientManager* allocate(Ami::AbsClient&);
      const Ami::DiscoveryRx& rx() const;
    public:
      void connected       () ;
      int  configure       (iovec*) ;
      int  configured      () ;
      void discovered      (const DiscoveryRx&) ;
      int  read_description(Socket&,int) ;
      int  read_payload    (Socket&,int) ;
      bool svc             () const ;
      void process         () ;

    private:
      unsigned       _interface;
      unsigned       _serverGroup;
      ConnectionManager* _connect_mgr;
      ClientManager* _manager;
      mutable Ami::Semaphore _discover_sem;
      const DiscoveryRx* _pdiscovery;
    };
  };
};

#endif
