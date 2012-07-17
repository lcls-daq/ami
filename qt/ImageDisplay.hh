#ifndef AmiQt_ImageDisplay_hh
#define AmiQt_ImageDisplay_hh

#include "ami/qt/Display.hh"
#include "ami/qt/ImageClient.hh"
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenuBar>
#include <QGroupBox>
#include "ami/service/Semaphore.hh"
#include <list>

class QLabel;
class QPrinter;
class ImageClient;

namespace Ami {
  namespace Qt {
    class ImageGridScale;
    class ImageColorControl;
    class ImageFrame;
    class ImageDisplay : public QWidget,
			 public Display {
      Q_OBJECT
    public:
      ImageDisplay(bool grab=true);
      ~ImageDisplay();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void prototype(const Ami::DescEntry*) {}
      void add   (QtBase*, bool);
      void reset ();
      void show  (QtBase*);
      void hide  (QtBase*);
      const AbsTransform& xtransform() const;
      void update();
      bool canOverlay() const { return false; }
      QWidget* widget() { return this; }
    public:
      void container(ImageClient* c) { _container = c; }
      const ImageColorControl& control() const;
      ImageGridScale& grid_scale();
    public:
//       const std::list<QtBase*> plots() const;
//       const AxisArray&    xinfo     () const;
      ImageFrame*          plot      () const;
    public slots:
      void save_image();
      void save_data();
      void save_reference();
      void hide_chrome();
      void show_chrome();
      void update_timedisplay();
    signals:
      void redraw();
    private:
      void _layout();
    private:
      ImageClient* _container;
      QLabel* _time_display;
      ImageFrame*  _plot;
      QMenuBar*    _menu_bar;
      QGroupBox*   _plotBox;
      QVBoxLayout* _mainLayout;
      QHBoxLayout* _layout2;
      ImageGridScale*    _units;
      ImageColorControl* _zrange;
      std::list<QtBase*>  _curves;
      std::list<QtBase*>  _hidden;
      mutable Ami::Semaphore _sem;
      unsigned               _resizeCount;
    };
  };
};


#endif
