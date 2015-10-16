#ifndef Ami_PVWriter_hh
#define Ami_PVWriter_hh

#include "EpicsCA.hh"

namespace Ami_Epics {
  class PVWriter : public EpicsCA {
  public:
    PVWriter(const char* pvName) : EpicsCA(pvName,0) {}
    ~PVWriter() {}
  public:
    void  put() { if (connected()) _channel.put(); }
  };

};

#endif
