#ifndef AmiQt_QtOverlay_hh
#define AmiQt_QtOverlay_hh

#include <QtCore/QObject>

namespace Ami {
  namespace Qt {
    class OverlayParent;
    class QtBase;
    class QtPlot;
    class QtOverlay : public QObject {
      Q_OBJECT
    public:
      QtOverlay(OverlayParent&   parent);
      virtual ~QtOverlay();
    public:
      virtual void dump(FILE*) const = 0;
      virtual const QtBase* base() const = 0;
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
		 
