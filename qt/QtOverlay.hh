#ifndef AmiQt_QtOverlay_hh
#define AmiQt_QtOverlay_hh

#include <QtCore/QObject>

namespace Ami {
  namespace Qt {
    class OverlayParent;
    class QtPlot;
    class QtOverlay : public QObject {
      Q_OBJECT
    public:
      QtOverlay(OverlayParent&   parent);
      virtual ~QtOverlay();
    protected:
      void attach(QtPlot&);
    signals:
      void changed();
    private:
      OverlayParent&     _parent;
    };
  };
};

#endif
		 
