#ifndef Ami_PVMonitorCb_hh
#define Ami_PVMonitorCb_hh

namespace Ami_Epics {
  class PVMonitorCb {
  public:
    virtual ~PVMonitorCb() {}
    virtual void updated() = 0;
  };
};

#endif
