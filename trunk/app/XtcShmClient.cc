#include <stdlib.h>
#include <stdio.h>

#include "ami/app/XtcShmClient.hh"
#include "ami/app/XtcClient.hh"
#include "ami/service/Routine.hh"
#include "ami/service/Task.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/TransitionId.hh"

//#define DBUG

namespace Ami {
  class ShmTask : public Routine {
  public:
    ShmTask(XtcShmClient& c, char* tag, int id) : 
      _c(c), _tag(tag), _id(id) {}
    ~ShmTask() {}
  public:
    void routine() { _c.run(_tag,_id,_id); delete this; }
  private:
    XtcShmClient& _c;
    char*         _tag;
    int           _id;
  };
};

using namespace Ami;

XtcShmClient::XtcShmClient(XtcClient& client, char * tag, int id) :
  _client(client),
  _sem(Semaphore::EMPTY),
  _task(new Task(TaskObject("amishm")))
{
  if (::pipe(_pipefd)<0)
    perror("XtcShmClient pipe failed");

  _task->call(new ShmTask(*this,tag,id));
}

int XtcShmClient::processDgram(Dgram* dg)
{
  ::write(_pipefd[1],&dg,sizeof(dg));
  _sem.take();
  return 0;
}

int XtcShmClient::processIo()
{
  Dgram* dg;
  ::read(_pipefd[0],&dg,sizeof(dg));
  _client.processDgram(dg);
#ifdef DBUG
  printf("XSC: processIo %s %d.%09d\n",
         TransitionId::name(dg->seq.service()),
         dg->seq.clock().seconds(),
         dg->seq.clock().nanoseconds());
#endif
  bool event = dg->seq.service()==TransitionId::L1Accept;
  _sem.give();
  if (event)
    for(std::list<Routine*>::iterator it=_list.begin(); it!=_list.end(); it++)
      (*it)->routine();

  return 1;
}

int XtcShmClient::fd() const { return _pipefd[0]; }

