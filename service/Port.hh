#ifndef Ami_MonPort_hh
#define Ami_MonPort_hh

namespace Ami {

  class Port {
  public:
    static unsigned short clientPort();
    static unsigned short serverPortBase();
    static unsigned short nServerPorts();
  };
};

#endif
