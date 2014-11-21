#ifndef AmiQt_QtOverlay_hh
#define AmiQt_QtOverlay_hh

#include <QtCore/QObject>

#include "ami/data/Cdu.hh"

namespace Ami {
  class Cds;
  namespace Qt {
    class OverlayParent;
    class QtBase;
    class QtPlot;
    class QtOverlay : public QObject,
		      public Cdu {
      Q_OBJECT
    public:
      QtOverlay(OverlayParent&   parent);
      virtual ~QtOverlay();
    public:
      virtual void dump(FILE*) const = 0;
      virtual const QtBase* base() const = 0;
    public:
      void clear_payload();
    protected:
      void attach(QtPlot&,Cds&);
    signals:
      void changed();
      void updated();
    private:
      OverlayParent&     _parent;
      QtPlot*            _plot;
    };
  };
};

#endif
		 
