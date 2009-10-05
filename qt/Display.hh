#ifndef AmiQt_Display_hh
#define AmiQt_Display_hh

#include "ami/qt/QtPersistent.hh"

class QWidget;

namespace Ami {
  class AbsTransform;
  namespace Qt {
    class QtBase;
    class Display : public QtPersistent {
    public:
      virtual ~Display() {}
    public:
      virtual void add   (QtBase*) = 0;
      virtual void reset () = 0;
      virtual void show  (QtBase*) = 0;
      virtual void hide  (QtBase*) = 0;
      virtual const AbsTransform& xtransform() const = 0;
      virtual void update() = 0;
      virtual bool canOverlay() const = 0;
      virtual QWidget* widget() = 0;
    };
  };
};


#endif
