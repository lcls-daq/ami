#ifndef AmiQt_RectROI_hh
#define AmiQt_RectROI_hh

#include "ami/qt/CPostParent.hh"
#include "ami/qt/OverlayParent.hh"

#include "ami/data/ConfigureRequestor.hh"
#include "ami/service/Semaphore.hh"

#include <list>

#include <QtCore/QString>
#include <QtCore/QObject>

class QWidget;

namespace Ami {
  class AbsOperator;
  class BinMath;
  class Cds;
  namespace Qt {
    class ChannelDefinition;
    class Rect;
    class ProjectionPlot;
    class CursorPlot;
    class CursorPost;
    class CursorOverlay;
    class ZoomPlot;

    class RectROI : public QObject,
                    public CPostParent,
                    public OverlayParent {
      Q_OBJECT
    public:
      RectROI(QWidget*, const QString&, unsigned ch, const Rect&);
      ~RectROI();
    public:
      const Rect& rect() const { return _rect; }
      unsigned channel() const { return _channel; }
      unsigned signature() const { return _signature; }
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public:
      void remove_cursor_post(CursorPost*);
    public:
      void      add_projection (AbsOperator*);
      void      add_cursor_plot(BinMath*);
      void      add_zoom_plot  ();
      void      request_roi    (bool);
    public:
      QString   add_post      (const QString&, const char*);
      QString   add_post      (const QString&, const char*, SharedData*&);
      void      add_overlay   (QtPlot&, BinMath*);
      void      add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void      remove_overlay(QtOverlay*);
    public slots:
      void remove_plot(QObject*);
    signals:
      void changed();
    private:
      QWidget*           _parent;
      QString            _name;
      unsigned           _channel;
      unsigned           _signature;
      const Rect&        _rect;
      ConfigureRequestor _req;
      Semaphore          _list_sem;
      std::list<ProjectionPlot*> _pplots;
      std::list<CursorPlot*>     _cplots;
      std::list<ZoomPlot*>       _zplots;
      std::list<CursorPost*>     _posts;
      std::list<CursorOverlay*>  _ovls;
      bool                       _roi_requested;
    };

  };
};

#endif
