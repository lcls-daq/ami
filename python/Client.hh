#ifndef AmiPython_Client_hh
#define AmiPython_Client_hh

#include "ami/client/AbsClient.hh"
#include "ami/data/Cds.hh"
#include "ami/service/Pool.hh"

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

    struct ClientArgs {
      Pds::DetInfo info; 
      unsigned     channel;
      AbsFilter*   filter;
      AbsOperator* op;
    };
    
    class Client : public Ami::AbsClient {
    public:
      Client(const std::vector<ClientArgs>& args);
      virtual ~Client();
    public:
      enum { Success, TimedOut, NoEntry };
      int  initialize      (ClientManager&);
      int  request_payload (int tmo_sec=2);
      std::vector<const Entry*> payload () const;
      void reset           ();
    public:      //  Push mode semantics
      void pstart();
      int  pget  (int tmo_sec=2);
      void pnext ();
      void pstop ();
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
      std::vector<ClientArgs> _args;
      std::vector<unsigned  > _input;
      std::vector<unsigned  > _output;

      Pool        _request;
      Pool        _description;

      Cds             _cds;
      ClientManager*  _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      bool            _described;
      pthread_cond_t  _described_cond;

      sem_t           _initial_sem;

      pthread_mutex_t _payload_mutex;
      pthread_cond_t  _payload_cond_avail;
      pthread_cond_t  _payload_cond_unavail;
      bool            _payload_avail;
    };
  };
};

#endif
