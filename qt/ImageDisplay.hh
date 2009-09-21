#ifndef AmiQt_ImageDisplay_hh
#define AmiQt_ImageDisplay_hh

#include "ami/qt/Display.hh"
#include <QtGui/QWidget>

#include <list>

namespace Ami {
  namespace Qt {
    class ImageColorControl;
    class ImageFrame;
    class ImageDisplay : public QWidget,
			 public Display {
      Q_OBJECT
    public:
      ImageDisplay();
      ~ImageDisplay();
    public:
      void add   (QtBase*);
      void reset ();
      void show  (QtBase*);
      void hide  (QtBase*);
      const AbsTransform& xtransform() const;
      void update();
      bool canOverlay() const { return false; }
      QWidget* widget() { return this; }
    public:
//       const std::list<QtBase*> plots() const;
//       const AxisArray&    xinfo     () const;
      ImageFrame*          plot      () const;
    public slots:
      void save_image();
      void save_data();
      void save_reference();
    signals:
      void redraw();
    private:
      ImageFrame*  _plot;
      ImageColorControl* _zrange;
      std::list<QtBase*>  _curves;
      std::list<QtBase*>  _hidden;
    };
  };
};


#endif
