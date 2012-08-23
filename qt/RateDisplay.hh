#ifndef AmiQt_RateDisplay_hh
#define AmiQt_RateDisplay_hh

#include "ami/service/Timer.hh"

#include "ami/data/Cds.hh"

class iovec;
class QVBoxLayout;

namespace Ami {
  class DiscoveryRx;
  class Socket;
  class ClientManager;
  class ConnectionManager;
  namespace Qt {
    class RateCalculator;
    class RecvCalculator;
    class RateDisplay : public Timer {
    public:
      RateDisplay(ConnectionManager&,ClientManager*);
      ~RateDisplay();
    public:
      void addLayout       (QVBoxLayout*);
      int  configure       (char*&);
      void discovered      (const DiscoveryRx&);
      void read_description(Socket&,int);
      int  read_payload    (Socket&,int);
      void process         ();
    public:  // Timer interface
      void     expired();
      Task*    task   () { return _task; }
      unsigned duration  () const { return 1000; }
      unsigned repetitive() const { return 1; }
    private:
      ClientManager*  _manager;
      unsigned        _input;
      char*           _description;
      Cds             _cds;
      unsigned        _niovload;
      iovec*          _iovload;
      RateCalculator* _inputCalc;
      RateCalculator* _acceptCalc;
      RecvCalculator* _netCalc;
      Task*           _task;
      volatile bool   _ready;
    };
  };
};

#endif
