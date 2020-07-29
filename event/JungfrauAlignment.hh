#ifndef Ami_JungfrauAlignment_hh
#define Ami_JungfrauAlignment_hh

#include "ami/event/Alignment.hh"

namespace Ami {
  namespace Alignment {
    class Jungfrau : public Detector {
    public:
      Jungfrau(const Pds::DetInfo& det, unsigned nelems,
               unsigned rows, unsigned columns, unsigned index=0);
      virtual ~Jungfrau();
    };
  }
}

#endif
