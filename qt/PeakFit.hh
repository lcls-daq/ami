#ifndef AmiQt_PeakFit_hh
#define AmiQt_PeakFit_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/PFPostParent.hh"
#include "ami/qt/OverlayParent.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QVBoxLayout;
class QComboBox;
class QTabWidget;

#include "ami/qt/Cursors.hh"
#include "ami/data/ConfigureRequest.hh"

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class EdgeCursor;
    class PeakFitPlot;
    class PeakFitPost;
    class PeakFitOverlay;
#if 0
    class DescTH1F;
    class DescProf;
    class DescScan;
    class DescChart;
#else
    class ScalarPlotDesc;
#endif
    class WaveformDisplay;
    class CursorLocation;
    class CursorDefinition;

    class PeakFit : public QtPWidget,
                    public Cursors,
                    public PFPostParent,
                    public OverlayParent {
      Q_OBJECT
    public:
      PeakFit(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, 
              WaveformDisplay&, QtPWidget* =0);
      ~PeakFit();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void remove(CursorDefinition&);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel(int); // set the source
      void set_quantity(int); // set the parameter
      void plot        ();   // configure the plot
      void overlay     ();   // configure the plot
      void remove_plot (QObject*);
      void add_post    ();
      void grab_cursorx();
      void add_cursor  ();
    signals:
      void changed();
      void grabbed();
    private:
      QString _add_post    ();
    public:
      void mousePressEvent  (double, double);
      void mouseMoveEvent   (double, double);
      void mouseReleaseEvent(double, double);
    private:
      enum { MAX_BINS = 10 };
      QStringList     _names;
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      WaveformDisplay&  _frame;
      QtPWidget*        _frameParent;
      EdgeCursor* _baseline;
      unsigned   _quantity;

      QTabWidget* _baseline_tab;

      QLineEdit* _title;
      CursorLocation *_lvalue;

#if 0
      QTabWidget* _plottype_tab;
      DescTH1F*  _hist;
      DescChart* _vTime;
      DescProf*  _vFeature;
      DescScan*  _vScan;
#else
      ScalarPlotDesc* _scalar_desc;
#endif

      std::list<PeakFitPlot*> _plots;
    public:
      void remove_peakfit_post(PeakFitPost*);
    private:
      std::list<PeakFitPost*> _posts;
      std::list<CursorDefinition*> _cursors;

      QVBoxLayout*    _clayout;

    public:
      void add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void remove_overlay(QtOverlay*);
    private:
      std::list<PeakFitOverlay*> _ovls;
    };
  };
};

#endif
