#include "ami/app/XtcClientT.hh"
#include "ami/service/Task.hh"
#include "ami/service/Routine.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/TransitionId.hh"

static const unsigned _depth=4;

namespace AmiT {
  class EventRoutine : public Ami::Routine {
  public:
    EventRoutine(Ami::XtcClient& c, 
                 char*           dg,
                 sem_t*          sem) : _c(c), _dg(dg), _sem(sem) {}
    void routine() { 
      Pds::Dgram* pdg = reinterpret_cast<Pds::Dgram*>(_dg);
      _c.Ami::XtcClient::processDgram(pdg); 
      sem_post(_sem); 
      delete[] _dg;
      delete this;
    }
  private:
    Ami::XtcClient& _c;
    char*           _dg;
    sem_t*          _sem;
  };
};

using namespace Ami;
using namespace Pds;

XtcClientT::XtcClientT(std::vector<FeatureCache*>& cache, 
                       Factory&      factory, 
                       EventFilter&  filter,
                       bool          sync) :
  XtcClient(cache,factory,filter,sync),
  _task(new Task(TaskObject("xtcclient"))),
  _tsem(new sem_t),
  _sem (0)
{
  sem_init(_tsem,0,1);
}

XtcClientT::~XtcClientT() {}

void XtcClientT::processDgram(Pds::Dgram* dg)
{
  unsigned sz = dg->xtc.sizeofPayload()+sizeof(*dg);
  char* ndg = new char[sz];
  memcpy(ndg,dg,sz);

  TransitionId::Value id=dg->seq.service();
  if (id==TransitionId::L1Accept) {
    _task->call(new AmiT::EventRoutine(*this,ndg,_sem));
    sem_wait(_sem);
  }
  else {
    if (_sem) {
      for(unsigned i=0; i<_depth; i++)
        sem_wait(_sem);
      sem_destroy(_sem);
      delete _sem;
      _sem = 0;
    }
    if (id==TransitionId::Enable) {
      _sem = new sem_t;
      sem_init(_sem,0,_depth);
    }
    sem_wait(_tsem);
    _task->call(new AmiT::EventRoutine(*this,ndg,_tsem));
  }
}

void XtcClientT::wait()
{
  sem_wait(_tsem);
  sem_post(_tsem);
}
