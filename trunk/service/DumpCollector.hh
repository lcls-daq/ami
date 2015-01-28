#ifndef Ami_DumpCollector_hh
#define Ami_DumpCollector_hh

#include "ami/service/Routine.hh"
#include "ami/service/VServerSocket.hh"
#include "ami/service/Ins.hh"

#include <list>

namespace Ami {
  class DumpSource;
  class Task;
  class DumpCollector : public Routine {
  public:
    DumpCollector(int interface=0x7f000001,
                  int secondary=0);
    ~DumpCollector();
  public:
    void add(const DumpSource&);
    void routine();
    static Ins ins();
  private:
    Task*                  _task;
    VServerSocket          _srv;
    int                    _sec;
    std::list<const DumpSource*> _servers;
  };
};

#endif
