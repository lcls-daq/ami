#include "ami/qt/QtMonitorClient.hh"

class QtMonitorServer : public XtcMonitorServer {
private:
  queue<Dgram*> _pool;
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

void QtMonitorServer::_deleteDatagram(Dgram* dg) {
  _pool.push(dg); 
}

QtMonitorServer::QtMonitorServer(const char* tag,
                                 unsigned sizeofBuffers, 
                                 unsigned numberofEvBuffers, 
                                 unsigned numberofClients,
                                 unsigned sequenceLength,
                                 QtMonitorClient* client) :
  XtcMonitorServer(tag,
                   sizeofBuffers,
                   numberofEvBuffers,
                   numberofClients,
                   sequenceLength),
  _client(client) {
  //  sum of client queues (nEvBuffers) + clients + transitions + shuffleQ
  unsigned depth = 2*numberofEvBuffers+XtcMonitorServer::numberofTrBuffers+numberofClients;
  for(unsigned i=0; i<depth; i++)
    _pool.push(reinterpret_cast<Dgram*>(new char[sizeofBuffers]));
}

QtMonitorServer::~QtMonitorServer() {
  while(!_pool.empty()) {
    delete _pool.front();
    _pool.pop();
  }
}

XtcMonitorServer::Result QtMonitorServer::events(Dgram* dg) {
  Xtc xtc = dg->xtc;
  if (XtcMonitorServer::events(dg) == XtcMonitorServer::Handled) {
    _deleteDatagram(dg);
  }
  return XtcMonitorServer::Deferred;
}

// Insert a simulated transition
void QtMonitorServer::insert(TransitionId::Value tr) {
  Dgram* dg = _pool.front(); 
  _pool.pop(); 
  new((void*)&dg->seq) Sequence(Sequence::Event, tr, ClockTime(0,0), TimeStamp(0,0,0,0));
  new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Event,0,0));
  _client->printTransition(dg);
  events(dg);
}
