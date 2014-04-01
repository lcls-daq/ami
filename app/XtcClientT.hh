#ifndef Ami_XtcClientT_hh
#define Ami_XtcClientT_hh

#include "ami/app/XtcClient.hh"

#include <semaphore.h>

namespace Ami {
  class Task;
  class Routine;

  class XtcClientT : public XtcClient {
  public:
    XtcClientT(std::vector<FeatureCache*>& cache, 
               Factory&      factory, 
               EventFilter&  filter,
               bool          sync=false);
    ~XtcClientT();
  public:
    void processDgram(Pds::Dgram*);
  private:
    Task*  _task;
    sem_t* _tsem;
    sem_t* _sem;
  };
};

#endif
