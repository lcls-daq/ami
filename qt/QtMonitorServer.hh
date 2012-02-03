#ifndef Ami_QtMonitorServer_hh
#define Ami_QtMonitorServer_hh

#include "pdsdata/app/XtcMonitorServer.hh"
#include "ami/qt/QtMonitorClient.hh"

using Pds::Dgram;
using Pds::TransitionId;
using Pds::XtcMonitorServer;

namespace Ami {
  namespace Qt {
    class QtMonitorServer : public XtcMonitorServer {
    private:
      std::queue<Dgram*> _pool;
      void _deleteDatagram(Dgram* dg);
      QtMonitorClient* _client;
    public:
      QtMonitorServer(const char* tag,
                      unsigned sizeofBuffers, 
                      unsigned numberofEvBuffers, 
                      unsigned numberofClients,
                      unsigned sequenceLength,
                      QtMonitorClient* client);
      ~QtMonitorServer();
      XtcMonitorServer::Result events(Dgram* dg);
      void insert(TransitionId::Value tr);
    };
  };
};

#endif
