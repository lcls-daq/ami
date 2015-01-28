#ifndef AmiQt_VAConfigApp_hh
#define AmiQt_VAConfigApp_hh

#include "ami/qt/OverlayParent.hh"
#include "ami/qt/CPostParent.hh"

#include "ami/data/ConfigureRequestor.hh"
#include "ami/service/Semaphore.hh"

#include <QtCore/QObject>
#include <QtCore/QString>

class QWidget;

namespace Ami {
  class AbsOperator;
  class BinMath;
  class Cds;

  namespace Qt {
    class ZoomPlot;
    class PeakPlot;
    class CursorPlot;
    class CursorPost;
    class CursorOverlay;
    class ChannelDefinition;

    class VAConfigApp : public QObject,
			public OverlayParent,
			public CPostParent {
      Q_OBJECT
    public:
      VAConfigApp(QWidget* parent, 
		  const QString&, 
		  unsigned i);
      VAConfigApp(QWidget* parent,
		  const char*&);
      virtual ~VAConfigApp();
    protected:
      virtual Ami::AbsOperator* _op(const char* name) = 0;
    public:
      const QString name   () const { return _name; }
      unsigned      channel() const { return _channel; }
    public:
      void add_map        (Ami::AbsOperator*);   
      void add_cursor_plot(BinMath*);   
      QString add_post    (const QString&, const char*, SharedData*&);   
      void add_overlay    (QtPlot&,BinMath*);
      void add_overlay    (DescEntry*,QtPlot*,SharedData*);
      void remove_overlay (QtOverlay*);
      void remove_cursor_post(CursorPost*);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void save(char*&) const;
      void save_plots(const QString&) const;
    public slots:
      void remove_plot(QObject*);
    signals:
      void changed();
    private:
      QWidget*                   _parent;
      QString                    _name;
      unsigned                   _channel;
      unsigned                   _signature;
      ConfigureRequestor         _req;
      Semaphore                  _list_sem;
      std::list<ZoomPlot*>       _zplots;
      std::list<PeakPlot*>       _pplots;
      std::list<CursorPlot*>     _cplots;
      std::list<CursorPost*>     _posts;
      std::list<CursorOverlay*>  _ovls;
    };
  };
};

#endif
