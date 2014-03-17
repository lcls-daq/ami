#ifndef AmiPython_Handler_hh
#define AmiPython_Handler_hh

#include "ami/client/AbsClient.hh"

#include "ami/data/Cds.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <semaphore.h>
#include <pthread.h>
#include <vector>

namespace Ami {
  class AbsFilter;
  class AbsOperator;
  class ClientManager;
  class DescEntry;
  class Entry;

  namespace Python {

    class Handler : public Ami::AbsClient {
    public:
      Handler(const Pds::DetInfo& info, unsigned options=-1U);
      virtual ~Handler();
    public:
      enum { Success, TimedOut, NoEntry };
      int  initialize      (ClientManager&);
      unsigned get() const { return _options; }
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
      Pds::DetInfo _info;
      unsigned     _options;
      unsigned     _input;

      char*       _request
;
      ClientManager*  _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      sem_t           _initial_sem;
    };
  };
};

#endif
