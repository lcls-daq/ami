#ifndef AmiPython_L3TClient_hh
#define AmiPython_L3TClient_hh

#include "ami/client/AbsClient.hh"

#include <semaphore.h>

namespace Ami {
  class AbsFilter;
  class ClientManager;

  namespace Python {

    class L3TClient : public Ami::AbsClient {
    public:
      L3TClient(AbsFilter*, const char* fname);
      virtual ~L3TClient();
    public:
      enum { Success, TimedOut };
      int  initialize      (ClientManager&);
    public:      // AbsClient interface
      void managed         (ClientManager&);
      void connected       ();
      int  configure       (iovec*);
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      int  read_description(Ami::Socket&,int);
      int  read_payload    (Ami::Socket&,int);
      bool svc             () const;
      void process         ();
    private:
      AbsFilter*      _filter;
      const char*     _fname;
      char*           _request;
      ClientManager*  _manager;
      sem_t           _initial_sem;
    };
  };
};

#endif
