#ifndef Ami_EpixAlignment_hh
#define Ami_EpixAlignment_hh

#include "ami/event/Alignment.hh"

namespace Ami {
  namespace Alignment {
    class Epix10ka2M : public Detector {
    public:
      Epix10ka2M(const Pds::DetInfo& det, unsigned index=0);
      virtual ~Epix10ka2M();
    protected:
      void load_default();
    };

    class Epix10kaQuad : public Detector {
    public:
      Epix10kaQuad(const Pds::DetInfo& det, unsigned index=0);
      virtual ~Epix10kaQuad();
    protected:
      void load_default();
    };
  }
}

#endif
