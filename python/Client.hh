#ifndef AmiPython_Client_hh
#define AmiPython_Client_hh

#include "ami/client/AbsClient.hh"

#include "ami/data/Cds.hh"

#include "pdsdata/xtc/DetInfo.hh"

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
      int  request_payload ();
      std::vector<const Entry*> payload () const;
      void reset           ();
    public: // AbsClient interface
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

      char*       _request;
      char*       _description;

      Cds             _cds;
      ClientManager*  _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      sem_t           _initial_sem;
      sem_t           _payload_sem;
    };
  };
};

#endif
