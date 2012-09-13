#ifndef AmiQt_QtOverlay_hh
#define AmiQt_QtOverlay_hh

namespace Ami {
  namespace Qt {
    class OverlayParent;
    class QtPlot;
    class QtOverlay {
    public:
      QtOverlay(OverlayParent&   parent);
      virtual ~QtOverlay();
    protected:
      void attach(QtPlot&);
    private:
      OverlayParent&     _parent;
    };
  };
};

#endif
		 
