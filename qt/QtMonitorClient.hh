#ifndef Ami_QtMonitorClient_hh
#define Ami_QtMonitorClient_hh

#include "pdsdata/xtc/Dgram.hh"

namespace Ami {

class QtMonitorClient {
public:
  virtual void printTransition(const Pds::Dgram* dg, const double hz = 0) = 0;
  virtual ~QtMonitorClient() {
  }
};

};
#endif
