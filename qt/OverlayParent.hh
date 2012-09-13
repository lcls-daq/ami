#ifndef AmiQt_OverlayParent_hh
#define AmiQt_OverlayParent_hh

namespace Ami {
  class DescEntry;
  namespace Qt {
    class QtPlot;
    class QtOverlay;
    class SharedData;
    class OverlayParent {
    public:
      virtual ~OverlayParent() {}
    public:
      virtual void add_overlay(DescEntry*, QtPlot*, SharedData*) = 0;
      virtual void remove_overlay(QtOverlay*) = 0;
    };
  };
};

#endif
