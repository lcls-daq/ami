#ifndef AmiQt_VAConfigApp_hh
#define AmiQt_VAConfigApp_hh

#include "ami/qt/OverlayParent.hh"

#include "ami/data/ConfigureRequestor.hh"

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
			public OverlayParent {
      Q_OBJECT
    public:
      VAConfigApp(QWidget* parent, 
		  const QString&, 
		  unsigned i);
      virtual ~VAConfigApp();
    protected:
      virtual Ami::AbsOperator* _op(const char* name) = 0;
    public:
      void add_map        (Ami::AbsOperator*);   
      void add_cursor_plot(BinMath*);   
      void add_overlay    (QtPlot&,BinMath*);
      void add_overlay    (DescEntry*,QtPlot*,SharedData*);
      void remove_overlay (QtOverlay*);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void load(const char*&);
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
      std::list<ZoomPlot*>       _zplots;
      std::list<PeakPlot*>       _pplots;
      std::list<CursorPlot*>     _cplots;
      std::list<CursorPost*>     _posts;
      std::list<CursorOverlay*>  _ovls;
    };
  };
};

#endif
