#ifndef AmiQt_MaskDisplay_hh
#define AmiQt_MaskDisplay_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/data/ImageMask.hh"

#include <QtGui/QMenuBar>
#include <QtGui/QGroupBox>
#include <QtCore/QString>

#include <deque>

class QImage;
class QPolygon;
class QLabel;

namespace Ami {
  class EntryImage;
  namespace Qt {
    class ImageXYControl;
    class ImageColorControl;
    class MaskFrame;
    class QtBase;
    class QtImage;
    class RectHandle;
    class RingHandle;
    class PolyHandle;
    class PixlHandle;
    class MaskDisplay : public QtPWidget {
      Q_OBJECT
    public:
      MaskDisplay(bool grab=true);
      ~MaskDisplay();
    public:
      void setup(QtBase*,const QString&);
      void mouse_pos(int x, int y);
    public:
      void save(char*& p) const {}
      void load(const char*&) {}
    public slots:
      void clear_mask();
      void fill_mask();
      void invert_mask();
      void load_mask();
      void save_mask();
      void save_gain_map();
      void save_gain_map_2x2();
      void load_bkg ();
      void undo();
      void redo();
    signals:
      void redraw();
    private:
      void _layout();
    private:
      MaskFrame*     _plot;
      QMenuBar*      _menu_bar;
      QGroupBox*     _plotBox;
      ImageXYControl*    _xyrange;
      ImageColorControl* _zrange;
      QtImage*       _bkg;
      ImageMask*     _mask;
      EntryImage*    _entry;
      EntryImage*    _empty;
      QString        _fname;
      QLabel*        _mouse_x;
      QLabel*        _mouse_y;

    public slots:
      void toggle_bkg();
    private:
      QAction*       _bkg_action;
      bool           _bkg_is_visible;

    public slots:
      void toggle_comb();
    private:
      QAction*       _comb_action;
      bool           _comb_is_plus;

    public slots:
      void toggle_rect();
      void toggle_ring();
      void toggle_poly();
      void toggle_pixl();
      void apply_thrh();
    public:
      void push();
      void rect(int,int,int,int);
      void ring(int,int,int,int);
      void poly(const QPolygon&);
      void pixl(int,int,int,int);
      void thrh(int);
    private:
      QAction*       _rect_action;
      QAction*       _ring_action;
      QAction*       _poly_action;
      QAction*       _pixl_action;
      QAction*       _thrh_action;
      enum MaskAction { NoAction, RectAction, RingAction, PolyAction, PixlAction };
      MaskAction     _active_action;
      RectHandle*    _rect_handle;
      RingHandle*    _ring_handle;
      PolyHandle*    _poly_handle;
      PixlHandle*    _pixl_handle;

      QAction*       _undo_action;
      QAction*       _redo_action;
      enum { MaxQueue=10 };
      std::deque<ImageMask> _queue;
      unsigned              _queue_pos;
    };
  };
};


#endif
